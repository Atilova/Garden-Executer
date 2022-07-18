#ifndef _SERVER_H
  #define _SERVER_H

  #include <Arduino.h>
  #include <functional>
  #include <ESPAsyncWebServer.h>
  #include <AsyncWebSocket.h>
  #include <WiFi.h>
  #include <Secrete.h>
  #include <ArduinoJson.h>
  #include "SPIFFS.h"
  #include "Lightning.h"


  class WebsocketAPIServer
    {
      protected:
        constexpr static const uint16_t PORT = 80;

        static String templateProcessor(const String& varName)
          {
            if(varName == "VAR")
              return "12";

            return "";
          };

        static void onWsEventUpdate(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType wsType, void* arg, uint8_t* data, size_t len)
          {
            switch(wsType)
              {
                case WS_EVT_CONNECT:
                  {
                    // Serial.println("Connect");
                    break;
                  };

                case WS_EVT_DATA:
                  {
                    // Serial.println("Receive");
                    break;
                  };
                default:
                  break;
              };
          };

      public:
        AsyncWebSocket ws = AsyncWebSocket("/ws/");

      private:
        AsyncWebServer server = AsyncWebServer(PORT);
        SparkSensorController* sparkController;

        void connectToWifi()
          {
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            while(!WiFi.isConnected())
              delay(1000);

            Serial.printf("Running on -> http://%s:%d\r\n", WiFi.localIP().toString().c_str(), PORT);
          }

        void sendConfigJsonAPI(AsyncWebServerRequest *request)
          {
            char buffer[500];
            const char jsonTemplate[] = "{\"indoor\": %d, \"noiseLevel\": %d, \"showDisturber\": %d, \"watchdogThreshold\": %d, \"spike\": %d, \"lightningsCount\": %d}";

            SparkConfig* config = sparkController->getConfig();
            sprintf(buffer, jsonTemplate,
                  config->indoor,
                  config->noiseLevel,
                  config->showDisturber,
                  config->watchdogThreshold,
                  config->spike,
                  config->lightningsCount
                );

            request->send(200, "application/json", buffer);
         };

      void handleAnyRequest(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) 
        {
          if(request->url() == "/api/options" && request->method() == HTTP_POST)
            {
              StaticJsonDocument<500> jsonBuffer;
              DeserializationError error = deserializeJson(jsonBuffer, (const char*)data);
              if(!error && sparkController->updateConfig(jsonBuffer))
                return request->send(200);                  
            };

          request->send(400);
        };

        void addServerHandlers()
          {
            server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
              // if(!request->authenticate(PAGE_USERNAME, PAGE_PASSWORD))
                // return request->requestAuthentication();
              request->send(SPIFFS, "/index.html", "", false, templateProcessor);
            });

            server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
              request->send(401, "text/html", "<script>location.replace(\"/\");</script>");
            });

            server.on("/api/options", HTTP_GET, std::bind(&WebsocketAPIServer::sendConfigJsonAPI, this, std::placeholders::_1));

            server.onNotFound([](AsyncWebServerRequest *request) {
              request->send(404, "text/html", "<script>location.replace(\"/\");</script>");
            });

            server.serveStatic("/static/", SPIFFS, "/static/");
            
            server.onRequestBody(std::bind(
              &WebsocketAPIServer::handleAnyRequest, 
              this, 
              std::placeholders::_1, 
              std::placeholders::_2,
              std::placeholders::_3,
              std::placeholders::_4,
              std::placeholders::_5
            ));

            // ws.setAuthentication(PAGE_USERNAME, PAGE_PASSWORD);
            ws.onEvent(WebsocketAPIServer::onWsEventUpdate);

            server.addHandler(&ws);
          };

      public:
        WebsocketAPIServer(SparkSensorController& sparkController)
          {
            this->sparkController = &sparkController;
          };

        ~WebsocketAPIServer() {};

        void run()
          {
            addServerHandlers();
            connectToWifi();

            DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
            server.begin();
          };

        void notifyAll(JsonDocument& doc)
          {
            char buffer[500];
            serializeJson(doc, buffer);
            ws.textAll(buffer);
          };
    };
#endif