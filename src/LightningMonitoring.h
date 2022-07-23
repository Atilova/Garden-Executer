#ifndef _LIGHTNING_MONITORING_H_
  #define _LIGHTNING_MONITORING_H_

  #include <EEPROM.h>
  #include <SPIFFS.h>
  #include <Wire.h>
  #include <WiFi.h>
  #include <Secrete.h>
  #include <ArduinoJson.h>
  #include <ESPAsyncWebServer.h>
  #include <AsyncWebSocket.h>
  #include <FunctionalInterrupt.h>
  #include <Adafruit_BME280.h>
  #include <SparkFun_AS3935.h>


  #if !defined(SPARK_SENSOR_AS3935_ADDRESS)
    #define SPARK_SENSOR_AS3935_ADDRESS 0x03
  #endif

  #if !defined(BME280_WEATHER_SENSOR_ADDRESS)
    #define BME280_WEATHER_SENSOR_ADDRESS 0x76
  #endif

  #if !defined(EEPROM_CONFIG_ADDRESS)
    #define EEPROM_CONFIG_ADDRESS 0x0
  #endif


  namespace lightningMonitoring
    {
      struct SparkFunConfig
        {
          boolean valid = false;
          boolean indoor = true;  // == true - датчик расположен внутри дома, иначе -снаружи
          uint8_t noiseLevel = 7;  // По умолчанию 2, минимальный уровень шума сравнивается с известным эталонным напряжением. Если этот уровень превышен, микросхема выдает прерывание на вывод IRQ, сообщая, что не может работать должным образом из-за шума (INT_NH). .
          boolean showDisturber = true;  // == true - инфо о помехах вкл, false- не выводим инфо о наличие помех
          uint8_t watchdogThreshold = 6;  // Threshold значения 1-10. по умолчанию=2. чем больше значение, тем больше загрубляем и больше сопротивляемость помехам
          uint8_t spike = 6;  // Значение 1-11, значение по умолчанию =2, настройка, как и порог сторожевого таймера, может помочь определить между ложными событиями и действительной молнией.
                              // Форма пика анализируется во время процедуры проверки сигнала.
                              // УВЕЛИЧЕНИЕ ЭТОГО ЗНАЧЕНИЯ УВЕЛИЧИВАЕТ НАДЕЖНОСТЬ за счет чувствительности к отдаленным событиям.
                              // Это значение помогает различать события и реально молнию, анализируя форму всплеска во время процедуры проверки сигнала.
                              // чем больше значение, тем выше эффективность определения молнии(больше сопротивляемость помехам), но меньше дистанция определения молнии
                              // Всплеска импульса молнии должен быть больше этого порогового значения. !!! импульсы со значение ниже порога отбрасываются
          uint8_t lightningsCount = 1;  //  1,5,9, or 16.  Количество событий молнии перед IRQ установлено высоким. 15-ти минутное окно - окно времени до сброса количества
                                        // Обнаруженных событий молнии. Количество ударов молнии может быть установлено на 1,5,9 или 16.
        };


      class WeatherSensor
        {
          protected:
            constexpr static const uint8_t PASCALS_IN_MM_HG = 133.3223;

          private:
            Adafruit_BME280 weatherSensor;

            float temperature;
            uint8_t humidity;
            uint16_t pressure;

          public:
            WeatherSensor() {};
            ~WeatherSensor() {};

            void setup()
              {
                weatherSensor.begin(BME280_WEATHER_SENSOR_ADDRESS);
              };

            boolean measure(char* buffer, boolean diff=true)
              {
                if(!weatherSensor.sensorID())
                  return false;

                float currentTemperature = static_cast<float>(static_cast<int>(weatherSensor.readTemperature() * 10.)) / 10;
                uint8_t currentHumidity = round(weatherSensor.readHumidity());
                uint16_t currentPressure = round(weatherSensor.readPressure() / PASCALS_IN_MM_HG);

                if((diff && temperature == currentTemperature) && (humidity == currentHumidity && pressure == currentPressure))
                  return false;

                const char weatherTemplate[] = "{\"type\": \"WEATHER\", \"payload\": {\"temperature\": \"%.1f\", \"humidity\": \"%d\", \"pressure\": \"%d\"}}";

                temperature = currentTemperature;
                humidity = currentHumidity;
                pressure = currentPressure;

                sprintf(buffer, weatherTemplate, temperature, humidity, pressure);
                return true;
              };

        };

      class LightningMonitor
        {
          protected:
            constexpr static const uint16_t PORT = 80;
            static const uint16_t MEASURE_WEATHER_INTERVAL = 3000;

            static String templateProcessor(const String& varName);

          private:
            SparkFun_AS3935 sparkSensor = SparkFun_AS3935(SPARK_SENSOR_AS3935_ADDRESS);
            SparkFunConfig sparkConfig;

            uint8_t capacitor,
                    interruptPin;
            volatile boolean sparkHasDetected = true;

            WeatherSensor weatherSensor;
            uint32_t lastWeatherMeasuring = 0;

            AsyncWebServer server = AsyncWebServer(PORT);
            AsyncWebSocket ws = AsyncWebSocket("/ws/");

          protected:
            void IRAM_ATTR sparkUpdateISR(void);
            int checkAndUpdateConfig(JsonDocument& doc);

            void onWsEventUpdate(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType wsType, void* arg, uint8_t* data, size_t len);

          public:
            LightningMonitor(uint8_t capacitor, uint8_t interruptPin);
            ~LightningMonitor();
            void init(void);
            void loop(void);
            void run(void);

          private:
            void eraseEEPROM(void);
            void loadEEPROM(boolean erase=false);
            void updateEEPROM(void);
            void setupConfig(void);
            void showConfig(void);
            void ISRloop(void);

            void connectToWifi(void);
            void sendConfigJsonAPI(AsyncWebServerRequest *request);
            void handleBodyRequest(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
            void addServerHandlers(void);
            void notifyAll(JsonDocument& doc);
        };
    };
#endif