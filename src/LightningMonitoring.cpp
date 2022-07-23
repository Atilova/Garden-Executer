#include "LightningMonitoring.h"

using namespace lightningMonitoring;


LightningMonitor::LightningMonitor(uint8_t capacitor, uint8_t interruptPin)
  {
    this->capacitor = capacitor;
    this->interruptPin = interruptPin;
  };

LightningMonitor::~LightningMonitor() {};

String LightningMonitor::templateProcessor(const String& varName) 
  {
    return "";
  };

void IRAM_ATTR LightningMonitor::sparkUpdateISR(void)
  {
    sparkHasDetected = true;
  };

int LightningMonitor::checkAndUpdateConfig(JsonDocument& doc)
  {
    SparkFunConfig temporaryConfig = sparkConfig;

    if(doc.containsKey("indoor") && doc["indoor"].is<boolean>())
      temporaryConfig.indoor = doc["indoor"];

    if(doc.containsKey("noiseLevel") && doc["noiseLevel"].is<uint8_t>())
      {
        uint8_t anySetting = doc["noiseLevel"];
        if(anySetting < 0 || anySetting > 7)
          return -1;
        temporaryConfig.noiseLevel = anySetting;
      };

    if(doc.containsKey("showDisturber") && doc["showDisturber"].is<boolean>())
      temporaryConfig.showDisturber = doc["showDisturber"];

    if(doc.containsKey("watchdogThreshold") && doc["watchdogThreshold"].is<uint8_t>())
      {
        uint8_t anySetting = doc["watchdogThreshold"];
        if(anySetting < 1 || anySetting > 10)
          return -2;
        temporaryConfig.watchdogThreshold = anySetting;
      };

    if(doc.containsKey("spike") && doc["spike"].is<uint8_t>())
      {
        uint8_t anySetting = doc["spike"];
        if(anySetting < 1 || anySetting > 11)
          return -3;
        temporaryConfig.spike = anySetting;
      };

    if(doc.containsKey("lightningsCount") && doc["lightningsCount"].is<uint8_t>())
      {
        uint8_t anySetting = doc["lightningsCount"];
        if((anySetting != 1 && anySetting != 5) && (anySetting != 9 && anySetting != 16))
          return -4;
        temporaryConfig.lightningsCount = anySetting;
      };

    const char notification[] = "{\"type\": \"CONFIG_UPDATED\"}";    
    sparkConfig = temporaryConfig;
    
    delay(100);
    updateEEPROM();
    setupConfig();
    
    delay(100);
    ws.textAll(notification);
    return true;
  };

void LightningMonitor::onWsEventUpdate(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType wsType, void* arg, uint8_t* data, size_t len)
  {
    switch(wsType)
      {
        case WS_EVT_CONNECT:
          {
            char buffer[200];
            if(weatherSensor.measure(buffer, false));
              client->text(buffer);
            break;
          };

        case WS_EVT_DATA:
          {
            StaticJsonDocument<200> jsonBuffer;
            DeserializationError error = deserializeJson(jsonBuffer, (const char*)data);
            if(error || (!jsonBuffer.containsKey("type") || !jsonBuffer.containsKey("payload")))
              return;

            const char* type = jsonBuffer["type"].as<const char*>();
            if(!strcmp(type, "PING"))
              {
                char buffer[200];
                const char pongTemplate[] = "{\"type\": \"PONG\", \"payload\": \"%s\"}";
                const char* id = jsonBuffer["payload"].as<const char*>();
                sprintf(buffer, pongTemplate, id);
                client->text(buffer);
              };

            break;
          }
        default:
          break;
      };
  };

void LightningMonitor::init(void)
  {
    Wire.begin();    
    SPIFFS.begin();
    
    pinMode(interruptPin, INPUT_PULLUP);
    if(!sparkSensor.begin())
      {
        Serial.println("модуль Детектора молнии AS3935 не обнаружен!!!");
        return;
      };
    sparkSensor.tuneCap(capacitor);
    if(!sparkSensor.calibrateOsc())
      {
        Serial.println("Not Successfully Calibrated!");
        return;
      };

    loadEEPROM();  // true, если нужно очистить eeprom, иначе пусто
    setupConfig();
    showConfig();

    attachInterrupt(interruptPin, std::bind(&LightningMonitor::sparkUpdateISR, this), HIGH);

    weatherSensor.setup();
  };

void LightningMonitor::ISRloop(void)
  {
    if(!sparkHasDetected)
      return;

    sparkHasDetected = false;

    char buffer[300];
    const char mainResponseTemplate[] = "{\"type\": \"STATUS\", \"payload\": {\"status\": \"%s\", \"data\": %s}}";              

    switch(sparkSensor.readInterruptReg())
      {
        case NOISE_TO_HIGH:
          {
            Serial.println("Spark -> NOISE (Шум)");
            sprintf(buffer, mainResponseTemplate, "NOISE", "null");
            break;
          };
        case DISTURBER_DETECT:
          {
            Serial.println("Spark -> DISTURBER (Помеха)");
            sprintf(buffer, mainResponseTemplate, "DISTURBER", "null");
            break;
          };
        case LIGHTNING:
          {
            uint8_t lightningDistance = sparkSensor.distanceToStorm();
            uint32_t lightningEnergy = sparkSensor.lightningEnergy();

            Serial.println("Spark -> LIGHTNING (молния)");

            char subBuffer[150];
            const char dataTemplate[] = "{\"distance\": %d, \"energy\": %d}";
            sprintf(subBuffer, dataTemplate, lightningDistance, lightningEnergy);
            sprintf(buffer, mainResponseTemplate, "LIGHTNING", subBuffer);
            break;
          };
        default:
          {
            Serial.println("Spark -> UNTRACKED: ");
            sprintf(buffer, mainResponseTemplate, "UNTRACKED", "null");
            break;
          };
      };
      ws.textAll(buffer);
  };

void LightningMonitor::eraseEEPROM(void)
  {
    EEPROM.write(EEPROM_CONFIG_ADDRESS, 0xff);
    EEPROM.commit();
  };

void LightningMonitor::loadEEPROM(boolean erase)
  {
    if(erase)
      eraseEEPROM();

    if(EEPROM.read(EEPROM_CONFIG_ADDRESS))
      updateEEPROM();
    else
      EEPROM.get(EEPROM_CONFIG_ADDRESS, sparkConfig);
  };

void LightningMonitor::updateEEPROM(void)
  {
    EEPROM.put(EEPROM_CONFIG_ADDRESS, sparkConfig);
    EEPROM.commit();
  };

void LightningMonitor::setupConfig(void)
  {
    Serial.println("SETUP CONFIG");
    sparkSensor.setIndoorOutdoor(sparkConfig.indoor ? INDOOR : OUTDOOR);
    sparkSensor.setNoiseLevel(sparkConfig.noiseLevel);
    sparkSensor.maskDisturber(!sparkConfig.showDisturber);
    sparkSensor.watchdogThreshold(sparkConfig.watchdogThreshold);
    sparkSensor.lightningThreshold(sparkConfig.lightningsCount);
    sparkSensor.spikeRejection(sparkConfig.spike);
    delay(10);
    sparkSensor.readInterruptReg();  // Requires first read, interrupt will not work otherwise
  };

void LightningMonitor::showConfig(void)
  {
    // Cчитываем значение indoor or outdoor из модуля
    Serial.print("IndoorOutdoor setup: ");
    Serial.println(sparkSensor.readIndoorOutdoor() == INDOOR ? "*Indoor*" : "*Outdoor*");

    // Cчитываем значение порогового значения шума из модуля
    Serial.print("Noise Level is set at: ");
    Serial.println(sparkSensor.readNoiseLevel());

    // Cчитываем из модуля инфо выдвать ли сообщения при помехе
    Serial.print("Помехи при работе будем выводить? -> ");
    sparkSensor.readMaskDisturber() ? Serial.println("нет") : Serial.println("да");

    // Считать значение сторожевого таймера
    Serial.print("Watchdog Threshold is set to: ");
    Serial.println(sparkSensor.readWatchdogThreshold());

    // Cчитываем пороговое значение всплеска импульса
    Serial.print("Spike Rejection is set to: ");
    Serial.println(sparkSensor.readSpikeRejection());

    Serial.print("Количество вспышек молнии до выдачи прерывания: ");
    Serial.println(sparkSensor.readLightningThreshold());  // Default is 0
  };

void LightningMonitor::connectToWifi(void)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(!WiFi.isConnected())
      delay(1000);

    Serial.printf("Running on -> http://%s:%d\r\n", WiFi.localIP().toString().c_str(), PORT);
  };

void LightningMonitor::sendConfigJsonAPI(AsyncWebServerRequest *request)
  {
    char buffer[400];
    const char configTemplate[] = "{\"indoor\": %d, \"noiseLevel\": %d, \"showDisturber\": %d, \"watchdogThreshold\": %d, \"spike\": %d, \"lightningsCount\": %d}";

    sprintf(
      buffer,
      configTemplate,
      sparkConfig.indoor,
      sparkConfig.noiseLevel,
      sparkConfig.showDisturber,
      sparkConfig.watchdogThreshold,
      sparkConfig.spike,
      sparkConfig.lightningsCount
      );
    request->send(200, "application/json", buffer);
  };

void LightningMonitor::handleBodyRequest(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total)
  {
    if(request->url() != "/api/options" || request->method() != HTTP_POST)
      return request->send(400);

      StaticJsonDocument<400> jsonBuffer;
      DeserializationError error = deserializeJson(jsonBuffer, (const char*)data);
      if(!error && checkAndUpdateConfig(jsonBuffer))
        return request->send(200);

    request->send(400);
  };

void LightningMonitor::addServerHandlers(void)
  {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      // if(!request->authenticate(PAGE_USERNAME, PAGE_PASSWORD))
        // return request->requestAuthentication();
      request->send(SPIFFS, "/index.html", "", false, templateProcessor);
    });

    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(401, "text/html", "<script>location.replace(\"/\");</script>");
    });

    server.on("/api/options", HTTP_GET, std::bind(&LightningMonitor::sendConfigJsonAPI, this, std::placeholders::_1));

    server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(404, "text/html", "<script>location.replace(\"/\");</script>");
    });

    server.serveStatic("/static/", SPIFFS, "/static/");

    server.onRequestBody(std::bind(
      &LightningMonitor::handleBodyRequest,
      this,
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4,
      std::placeholders::_5
    ));

     // ws.setAuthentication(PAGE_USERNAME, PAGE_PASSWORD);
    ws.onEvent(std::bind(
      &LightningMonitor::onWsEventUpdate,
      this,
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4,
      std::placeholders::_5,
      std::placeholders::_6
    ));
    server.addHandler(&ws);
  };

void LightningMonitor::run(void)
  {
    addServerHandlers();
    connectToWifi();

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();
  };

void LightningMonitor::notifyAll(JsonDocument& doc)
  {
    char buffer[500];
    serializeJson(doc, buffer);
    ws.textAll(buffer);
  };

void LightningMonitor::loop(void)
  {
    ISRloop();

    if(millis() - lastWeatherMeasuring > MEASURE_WEATHER_INTERVAL)
      {
        lastWeatherMeasuring = millis();
        char buffer[200];
        if(weatherSensor.measure(buffer))
          ws.textAll(buffer);
      };
  };