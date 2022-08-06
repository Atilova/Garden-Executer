#ifndef _MQTT_CONSUMER_H_
  #define _MQTT_CONSUMER_H_

  #include <Arduino.h>
  #include <PubSubClient.h>
  #include <Wifi.h>
  #include <ESP32Ping.h>
  #include <IPAddress.h>
  #include <Functions.h>
  #include <CircularBuffer.h>
  #include <functional>
  #include <ArduinoJson.h>
  #include <ESPRandom.h>


  namespace Mqtt
    {
      typedef std::function<void(char* topic, JsonDocument& doc)> receiveCallback_t;

      struct Config {
        IPAddress WIFI_IP;
        IPAddress WIFI_GATEWAY;
        IPAddress WIFI_SUBNET_MASK;
        IPAddress WIFI_PRIMARY_DNS;
        IPAddress WIFI_SECONDARY_DNS;
        const char* WIFI_SSID;
        const char* WIFI_PASSWORD;
        const char* SERVER_TOPIC_LISTEN;
        const char* SERVER_TOPIC_PUBLISH;
        const char** LISTENERS;
        const uint32_t LISTENERS_SIZE;
        const char* HOST;
        const uint16_t PORT;
        const uint8_t KEEPALIVE;
        const uint8_t SOCKET_TIMEOUT;
        const char* CLIENT_ID;
        const char* MQTT_USERNAME;
        const char* MQTT_PASSWORD;
      };

       struct PublishBufferData
        {
          const char* topic;
          const char* payload;
          boolean retain;
        };

      struct InboxBufferData
        {
          char* topic;
          byte* payload;
          uint8_t length;
        };

      class MqttConsumer : private PubSubClient
        {
          protected:
            constexpr static const char* HANDSHAKE_MESSAGE_TEMPLATE = "{\"type\": \"HANDSHAKE\", \"id\": \"%s\"}";
            constexpr static const char* PONG_MESSAGE_TEMPLATE = "{\"type\": \"PONG\"%s}";

            constexpr static const uint8_t WIFI_CONNECTION_TIMEOUT = 3,  // In Seconds
                                           WIFI_RETRY_ENTRIES = 5,  // Entries count, before WiFi will be reset
                                           NEXT_PING_DELAY = 2,  // In Seconds
                                           HANDSHAKE_TIMEOUT = 4;  // In Seconds

          private:
            Config* _conf;
            WiFiClient wifiClient;
            CircularBuffer<PublishBufferData, 100> publishBuffer;
            CircularBuffer<InboxBufferData, 20> inboxBuffer;
            StaticJsonDocument<1000> jsonBuffer;
            TaskHandle_t xEngineHandle = NULL;
            TaskHandle_t xPublisherHandle = NULL;
            TaskHandle_t xInboxHandle = NULL;
            receiveCallback_t userReceiveCallback;
            String lastId;
            void (MqttConsumer::* handleNext)(void) = &MqttConsumer::connectToWifi;
            volatile boolean isHandshaked = false;

            static void engineLoop(void* consumer)
              {
                MqttConsumer* _instance = static_cast<MqttConsumer*>(consumer);

                for(;;)
                  (_instance->*(_instance->handleNext))();
              };

            void clearWifiConnection(void)
              {
                WiFi.disconnect(true);
                WiFi.softAPdisconnect();
                WiFi.mode(WIFI_STA);
              };

            void connectToWifi(void)
              {
                const uint16_t timeoutDelay = calculateTicsTime(WIFI_CONNECTION_TIMEOUT*1000L);
                WiFi.config(_conf->WIFI_IP, _conf->WIFI_GATEWAY, _conf->WIFI_SUBNET_MASK, _conf->WIFI_PRIMARY_DNS, _conf->WIFI_SECONDARY_DNS);
                while(!WiFi.isConnected())
                  {
                    clearWifiConnection();
                    repeat(WIFI_RETRY_ENTRIES)
                      {
                        if(WiFi.isConnected())
                          break;
                        WiFi.begin(_conf->WIFI_SSID, _conf->WIFI_PASSWORD);
                        vTaskDelay(timeoutDelay);
                      };
                  };
                Serial.println("WIFI CONNECTED");
                handleNext = &MqttConsumer::pingServer;
              };

            void pingServer(void)
              {
                const uint16_t pingDelay = calculateTicsTime(NEXT_PING_DELAY*1000L);
                while(WiFi.isConnected())
                  {
                    if(Ping.ping(_conf->HOST, 3))
                      {
                        handleNext = &MqttConsumer::connectToServer;
                        return;
                      };
                    vTaskDelay(pingDelay);
                  };

                handleNext = &MqttConsumer::connectToWifi;
              };

            void connectToServer(void)
              {
                if(connect(_conf->CLIENT_ID, _conf->MQTT_USERNAME, _conf->MQTT_PASSWORD))
                  {
                    Serial.println("Mqtt Connected");
                    subscribe(_conf->SERVER_TOPIC_LISTEN);
                    for(uint8_t index = 0; index < _conf->LISTENERS_SIZE; index++)
                      subscribe(_conf->LISTENERS[index]);

                    callPublisher();
                    const uint16_t loopDelay = calculateTicsTime(100);

                    uint32_t lastTimeHandshaked = 0;
                    while(loop())
                      {
                        if(!isHandshaked && (millis() - lastTimeHandshaked) > HANDSHAKE_TIMEOUT*1000L)
                          {
                            lastTimeHandshaked = millis();

                            uint8_t uuid[16];
                            ESPRandom::uuid4(uuid);
                            lastId = ESPRandom::uuidToString(uuid);

                            char buffer[200];
                            sprintf(buffer, HANDSHAKE_MESSAGE_TEMPLATE, lastId.c_str());
                            publish(_conf->SERVER_TOPIC_PUBLISH, buffer);  // Use simple publish instead smartPublish to prevent garbage data after connection fail
                          };
                        vTaskDelay(loopDelay);
                      };
                  }
                  else
                    vTaskDelay(calculateTicsTime(1000));

                  isHandshaked = false;
                  handleNext = &MqttConsumer::pingServer;
              };

            void callPublisher(void)
              {
                if(xPublisherHandle != NULL)
                  return;

                xTaskCreate(
                  [](void* consumer)
                  {
                    MqttConsumer* _instance = static_cast<MqttConsumer*>(consumer);
                    const uint16_t delayBeforeNext = calculateTicsTime(100);
                    while(_instance->connected() && !_instance->publishBuffer.isEmpty())
                      {
                        PublishBufferData data = _instance->publishBuffer.pop();
                        _instance->publish(data.topic, data.payload, data.retain);
                        vTaskDelay(delayBeforeNext);
                      };
                    _instance->xPublisherHandle = NULL;
                    vTaskDelete(NULL);
                  },
                  "PUBLISHER-LOOP", 7000, (void*)this, 3, &xPublisherHandle
                );
              };

            boolean checkSystemMessage(const char* topic, JsonDocument& doc)
              {
                if(!doc.containsKey("type") || !doc["type"].is<const char*>())
                  return true;  // Skip, if data incorrect

                if(strcmp(_conf->SERVER_TOPIC_LISTEN, topic))
                  return false;  // Not main server topic

                const char* type = doc["type"];
                if(!strcmp(type, "HANDSHAKE"))
                  {
                    if(!doc.containsKey("id") || !doc["id"].is<const char*>())
                      return true;

                    const char* confirmationId = doc["id"];
                    if(strcmp(lastId.c_str(), confirmationId))
                      return true;

                    isHandshaked = true;  // Handshake done
                    return true;
                  }
                else if(!strcmp(type, "PING"))  // Requested PING -> send PONG
                  {
                    char buffer[200];

                    if(doc.containsKey("id") && doc["id"].is<const char*>())
                      {
                        char subBuffer[100];
                        const char idTemplate[] = "{\"id\": \"%s\"}";
                        const char* id = doc["id"];
                        sprintf(subBuffer, idTemplate, id);
                        sprintf(buffer, PONG_MESSAGE_TEMPLATE, subBuffer);
                      }
                      else
                        sprintf(buffer, PONG_MESSAGE_TEMPLATE, "");

                    publish(_conf->SERVER_TOPIC_PUBLISH, buffer);  // Use simple publish instead smartPublish to prevent garbage data after connection fail
                    return true;
                  };

                return false;
              };

            String uuid(void)
              {
                uint8_t uuid[16];
                ESPRandom::uuid(uuid);
                return ESPRandom::uuidToString(uuid);
              };

            void callReceiver(void)
              {
                if(xInboxHandle != NULL)
                  return;

                xTaskCreate(
                  [](void* consumer)
                  {
                    MqttConsumer* _instance = static_cast<MqttConsumer*>(consumer);
                    while(!_instance->inboxBuffer.isEmpty())
                      {
                        InboxBufferData data = _instance->inboxBuffer.pop();
                        deserializeJson(_instance->jsonBuffer, (const byte*)data.payload, data.length);
                        if(_instance->checkSystemMessage(data.topic, _instance->jsonBuffer))
                          continue;

                        _instance->userReceiveCallback(data.topic, _instance->jsonBuffer);
                      };

                    _instance->xInboxHandle = NULL;
                    vTaskDelete(NULL);
                  },
                  "INBOX-LOOP", 5000, (void*)this, 2, &xInboxHandle
                );
              };

            void onInboxCallback(char* topic, byte* payload, uint8_t length)
              {
                if(inboxBuffer.isFull())
                  return;
                inboxBuffer.push(InboxBufferData {topic, payload, length});
                callReceiver();
              };

          public:
            MqttConsumer(void) {};

            ~MqttConsumer(void) {};

            void configure(Config& config, receiveCallback_t callback)
              {
                this->_conf = &config;
                this->userReceiveCallback = callback;
                setClient(wifiClient);
                setServer(_conf->HOST, _conf->PORT);
                setKeepAlive(_conf->KEEPALIVE);
                setSocketTimeout(_conf->SOCKET_TIMEOUT);
                setCallback(std::bind(
                  &MqttConsumer::onInboxCallback,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3
                ));
              };

             boolean smartPublish(const char* topic, const char* payload, boolean retained)
              {
                if(publishBuffer.isFull())
                  return false;

                publishBuffer.push(PublishBufferData {topic, payload, retained});
                callPublisher();
                return true;
              };

            void run(void)
              {
                xTaskCreate(MqttConsumer::engineLoop, "MQTT-ENGINE", 3000, (void*)this, 1, &xEngineHandle);
              };
        };

      class TopicPublisher
        {
          private:
            const char* topic;
            MqttConsumer* consumer;

          public:
            TopicPublisher(const char* topic, MqttConsumer& consumer)
              {
                this->topic = topic;
                this->consumer = &consumer;
              };

            ~TopicPublisher() {};

            void yield(const char* payload, boolean retained=false)
              {
                consumer->smartPublish(topic, payload, retained);
              };
        };
    };
#endif