#ifndef _MQTT_CONSUMER_H_
  #define _MQTT_CONSUMER_H_

  #include <Arduino.h>
  #include <PubSubClient.h>
  #include <Wifi.h>
  #include <ESP32Ping.h>


  namespace mosquitto
    {
      #define MQTT_RECEIVE_CALLBACK std::function<void(char*, uint8_t*, unsigned int)> receiveCallback
      #define repeat(n) for(int i = n; i--;)
      // #define portMAX_DELAY TickType_t 0xffffffffUL

      struct Config {
        const char** listeners;
        const uint32_t listenersSize;
        const char* host;
        const uint16_t port;
        const uint8_t keepAlive;
        const uint8_t socketTimeout;
        const char* clientId;
        const char* mqttUsername;
        const char* mqttPassword;
        const char* wifiPoint;
        const char* wifiPassword;
      };


      class MqttConsumer : private PubSubClient
        {
          static const uint8_t wifiConnectionTimeout = 7,  // In Seconds
                               wifiRetryEntries = 3,  // Entries count, before WiFi will be reset
                               nextPingDelay = 2;  // In Seconds

          private:
            Config* _configuration;
            WiFiClient wifiClient;
            TaskHandle_t xEngineHandle = NULL;
            void (MqttConsumer::* handleNext)(void) = &MqttConsumer::connectToWifi;

            static void mainLoop(void* anyParameter)
              {
                MqttConsumer* _instance = (MqttConsumer*) anyParameter;
                for(;;)
                  (_instance->*(_instance->handleNext))();
              };

            uint32_t calculateTicsTime(uint32_t realTime)
              {
                return realTime / portTICK_PERIOD_MS;
              };

            void clearWifiConnection(void)
              {
                WiFi.disconnect(true);
                WiFi.softAPdisconnect();
                WiFi.mode(WIFI_STA);
              };

            void connectToWifi(void)
              {
                const uint16_t timeoutDelay = calculateTicsTime(wifiConnectionTimeout*1000L);
                while(!WiFi.isConnected())
                  {
                    clearWifiConnection();
                    repeat(wifiRetryEntries)
                      {
                        if(WiFi.isConnected())
                          break;
                        WiFi.begin(_configuration->wifiPoint, _configuration->wifiPassword);
                        vTaskDelay(timeoutDelay);
                      };
                  };
                Serial.println("WIFI CONNECTED");
                handleNext = &MqttConsumer::pingServerOverWifi;
              };

            void pingServerOverWifi(void)
              {
                const uint16_t pingDelay = calculateTicsTime(nextPingDelay*1000L);
                while(WiFi.isConnected())
                  {
                    if(Ping.ping(_configuration->host, 3))
                      break;
                    vTaskDelay(pingDelay);
                  };

                if(!WiFi.isConnected())
                  handleNext = &MqttConsumer::connectToWifi;
                else
                  handleNext = &MqttConsumer::connectToServer;
              };

            void connectToServer(void)
              {
                if(connect(_configuration->clientId, _configuration->mqttUsername, _configuration->mqttPassword))
                  {
                    Serial.println("Mqtt Connected");
                    for(uint8_t index = 0; index < _configuration -> listenersSize; index++)
                      subscribe(_configuration -> listeners[index]);

                    const uint16_t loopDelay = calculateTicsTime(200);
                    while(loop())
                      vTaskDelay(loopDelay);
                  }
                  else
                    vTaskDelay(calculateTicsTime(1000));

                  handleNext = &MqttConsumer::pingServerOverWifi;
              };

          public:
            MqttConsumer(void) {};
            ~MqttConsumer(void) {};

            void configure(Config& config)
              {
                this->_configuration = &config;
                setClient(wifiClient);
                setServer(config.host, config.port);
                setKeepAlive(config.keepAlive);
                setSocketTimeout(config.socketTimeout);
              };

            void start(void)
              {
                xTaskCreate(MqttConsumer::mainLoop, "MqttEngine", 3000, (void*)this, 1, &xEngineHandle);
              };

        };

    }
#endif