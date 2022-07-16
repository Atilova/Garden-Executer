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

        void connectToWifi()
          {
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            while(!WiFi.isConnected())
              delay(1000);

            Serial.printf("Running on -> http://%s:%d\r\n", WiFi.localIP().toString().c_str(), PORT);
          }

        void addServerHandlers()
          {
            server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
              if(!request->authenticate(PAGE_USERNAME, PAGE_PASSWORD))
                return request->requestAuthentication();
              request->send(SPIFFS, "/index.html", "", false, templateProcessor);
            });

            server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
              request->send(401, "text/html", "<script>location.replace(\"/\");</script>");              
            });

            server.onNotFound([](AsyncWebServerRequest *request) {
              request->send(404, "text/html", "<script>location.replace(\"/\");</script>");
            });

            server.serveStatic("/static/", SPIFFS, "/static/");
            ws.onEvent(WebsocketAPIServer::onWsEventUpdate);
            ws.setAuthentication(PAGE_USERNAME, PAGE_PASSWORD);
            server.addHandler(&ws);
          };
                
      public:
        WebsocketAPIServer() {};
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