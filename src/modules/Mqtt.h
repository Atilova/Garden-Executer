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


  namespace Mqtt
    {
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

      struct InboxBufferData
        {
          const char* topic;
          const byte* payload;
          uint8_t length;
        };

      struct PublishBufferData
        {
          const char* topic;
          const char* payload;
          boolean retain;
        };

      struct RequestParameters
        {
          public:
            constexpr static const byte UNUSED_BYTE = 0xff;
            
            // Request protocol allowed byte commands & arguments
            constexpr static const byte SENSOR_DEVICE = 0x0,  // deviceType
                                        RELAY_DEVICE = 0x1,
                                        ALL_DEVICES = 0x2;
            
            constexpr static const byte STATUS_OPERATION = 0x0,  // operationCode
                                        RUN_OPERATION = 0x1,
                                        SYNCHRONIZATION_OPERATION = 0x2,
                                        PING_OPERATION = 0x3;

            constexpr static const byte STATE_ON_ENABLE = 0x1,  // runParameter
                                        STATE_OFF_DISABLE = 0x0;
          protected:
            const unsigned char* _initialBytes;

          public:
            byte confirmationId[2];
            byte operationCode = UNUSED_BYTE,
                  deviceType = UNUSED_BYTE,  // 0x0: sensor; 0x1: relay
                  deviceId = UNUSED_BYTE,  // sensor or relay id number
                  runParameter = UNUSED_BYTE;
            boolean isError = true;

            RequestParameters(InboxBufferData& data)
              {
                if(data.length < 3)
                  return;

                _initialBytes = static_cast<const unsigned char*>(data.payload);

                assignBytesByIndex(0, confirmationId);
                assignByteByIndex(2, operationCode);
                // if(operationCode == UNUSED_BYTE || operationCode )

                if(!any_variants<byte>(
                  operationCode, 
                  STATUS_OPERATION, 
                  RUN_OPERATION, 
                  SYNCHRONIZATION_OPERATION,
                  PING_OPERATION)
                ) 
                  {
                    Serial.println("Incorrect operation");
                  };






                Serial.printf(
                  "Topic: %s, ConfirmationId: %x %x %x %x, OperationCode: %x\n",
                  data.topic,
                  confirmationId[0], confirmationId[1], confirmationId[2], confirmationId[3],
                  operationCode
                  );

                  isError = false;
              };

              ~RequestParameters() {};

            private:
              template<typename T, typename ...Vars> bool any_variants(T checkValue, Vars ...variants)
                {
                  return (... || (checkValue == variants));
                };

              void assignBytesByIndex(uint16_t byteStartIndex, byte* assignmentVariable, uint16_t nBytes=0)
                {
                  // function will use by default the full size - sizeof(assignmentVariable)
                  uint16_t getNBytes = nBytes ? nBytes : len(assignmentVariable);
                  memcpy(assignmentVariable, &_initialBytes[byteStartIndex], getNBytes);
                };

              void assignByteByIndex(uint16_t byteStartIndex, byte& assignmentVariable)
                {
                  byte result[1];
                  memcpy(result, &_initialBytes[byteStartIndex], 1);
                  assignmentVariable = result[0];
                };
        };

      class MqttConsumer : private PubSubClient
        {
          protected:
            constexpr static const uint8_t WIFI_CONNECTION_TIMEOUT = 3,  // In Seconds
                                           WIFI_RETRY_ENTRIES = 5,  // Entries count, before WiFi will be reset
                                           NEXT_PING_DELAY = 2,  // In Seconds
                                           HANDSHAKE_TIMEOUT = 4;  // In Seconds

            using stateHandle_t = void(MqttConsumer::*)(void);
            typedef std::function<void(InboxBufferData)> receiveCallback_t;

          private:
            Config* _conf;
            WiFiClient wifiClient;
            CircularBuffer<PublishBufferData, 100> publishBuffer;
            CircularBuffer<InboxBufferData, 20> inboxBuffer;
            TaskHandle_t xEngineHandle = NULL;
            TaskHandle_t xPublisherHandle = NULL;
            TaskHandle_t xInboxHandle = NULL;
            receiveCallback_t userReceiveCallback;
            stateHandle_t handleNext = &MqttConsumer::connectToWifi;

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

                    callPublisher();  // TODO; maybe need to be removed (send messages previously not sent )
                    const uint16_t loopDelay = calculateTicsTime(100);

                    while(loop())
                      vTaskDelay(loopDelay);
                  }
                  else
                    vTaskDelay(calculateTicsTime(1000));

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
                  "PUBLISHER-LOOP", 7000, static_cast<void*>(this), 3, &xPublisherHandle
                );
              };

            boolean checkSystemMessage(InboxBufferData& data)
              {
                if(strcmp(_conf->SERVER_TOPIC_LISTEN, data.topic))
                  return false;  // Not main server topic

                RequestParameters parameters(data);

                if(parameters.isError)
                  {
                    Serial.println("Error parsing parameters");
                    return true;
                  }

                //Serial.println(parameters.confirmationId, HEX);
                return true;
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
                        if(_instance->checkSystemMessage(data))
                          continue;

                        _instance->userReceiveCallback(data);
                      };

                    _instance->xInboxHandle = NULL;
                    vTaskDelete(NULL);
                  },
                  "INBOX-LOOP", 5000, static_cast<void*>(this), 2, &xInboxHandle
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
                xTaskCreate(MqttConsumer::engineLoop, "MQTT-ENGINE", 3000, static_cast<void*>(this), 1, &xEngineHandle);
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