#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <Adafruit_MCP23017.h>
  #include <DallasTemperature.h>
  #include <PZEM004Tv30.h>
  #include <Adafruit_BME280.h>
  #include "BaseTypes.h"

  #define repeat(n) for(int i = n; i--;)

  namespace systemSensors
    {
      struct AbstractSensor: public AbstractType
        {
          public:
            const char* name;

            AbstractSensor(const char* name)
              {
                this->name = name;
              };

            ~AbstractSensor() {};

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) {};

            // Will be called by Controller for every sensor, after controller.configure(...)
            // Use to run pinMode, setResolution, all stuff - requires builtin arduino setup(), to make initial begin
            virtual void setup(void) {};
        };


      class SensorsController : public BaseController
        {
          private:
            AbstractSensor** list;

          public:
            void configure(AbstractSensor** list, uint8_t size, const char* moduleNamespace, JsonDocument& doc)
              {
                BaseController::configure(size, moduleNamespace, doc);
                this->list = list;
                for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                    list[sensorIndex]->setup();
              };

            boolean measure(const char* name, boolean diff=true)
              {
                for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                  {
                    AbstractSensor* sensor = list[sensorIndex];
                    if(sensor->name == name)
                      {
                        sensor->measure(*doc, diff, moduleNamespace);
                        return true;
                      };
                  };
                  return false;
              };

            void measureAll(boolean diff=true) {
              for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                list[sensorIndex]->measure(*doc, diff, moduleNamespace);
            };
        };


      struct DigitalSensorI2C : public AbstractSensor
        {
          private:
            Adafruit_MCP23017* board;
            uint8_t pinOutput;
            boolean level;

          public:
            DigitalSensorI2C(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, uint8_t pinOutput, boolean level=LOW) :
              AbstractSensor(name)
              {
                this->board = &board;
                this->pinOutput = pinOutput;
                this->level = level;

                AbstractType::add(
                  new JsonFieldLevel({useType<boolean>(false), "", jsonLevels})
                );
              };

            virtual void setup(void) override
              {
                board->pinMode(pinOutput, INPUT_PULLUP);
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                uint8_t currentValue = board->digitalRead(pinOutput);
                setValue(doc, section, currentValue==level, diff);
              };
        };


      struct DallasTemperatureOneWireSensor : public AbstractSensor
        {
          protected:
            static const int ERROR_CODE = -127;

          private:
            DeviceAddress* deviceAddress;
            DallasTemperature* controller;
            uint8_t valueResolution;

          public:
            DallasTemperatureOneWireSensor(const char* name, const String& jsonLevels, DallasTemperature& controller, DeviceAddress& address, uint8_t resolution) :
              AbstractSensor(name)
              {
                this->controller = &controller;
                this->deviceAddress = &address;
                this->valueResolution = resolution;

                AbstractType::add(
                  new JsonFieldLevel({useType<float>(0.00), "", jsonLevels})
                );
              };

            virtual void setup(void) override
              {
                controller->setResolution(*deviceAddress, valueResolution);
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                controller->requestTemperatures();
                float currentValue = controller->getTempC(*deviceAddress);
                currentValue == ERROR_CODE ? setError(doc, section, diff) : setValue(doc, section, currentValue, diff);
              };
        };


      struct WaterFlowSensor : public AbstractSensor
        {
          private:
            uint8_t pinInput;

          public:
            WaterFlowSensor(const char* name, const String& jsonLevels, uint8_t pinInput) :
              AbstractSensor(name)
              {
                this->pinInput = pinInput;

                AbstractType::add(
                  new JsonFieldLevel({useType<uint16_t>(0), "", jsonLevels})
                );
              };

            virtual void setup(void) override
              {
                pinMode(pinInput, INPUT);
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                float totalPulse = pulseIn(pinInput, HIGH) + pulseIn(pinInput, LOW);
                if(!totalPulse)
                  setError(doc, section, diff);
                else
                  {
                    uint16_t currentValue = round(1000000 / totalPulse);
                    setValue(doc, section, currentValue, diff);
                  };
              };
        };


      struct WaterPressureSensor : public AbstractSensor
        {
          protected:
            static const uint8_t ZERO_PRESSURE_K = 0.17;
            static const uint16_t RESISTANCE_R1 = 1000,  // делитель 5v -> 3.3v
                                  RESISTANCE_R2 = 2000;
          private:
            uint8_t pinInput;

          public:
            WaterPressureSensor(const char* name, const String& jsonLevels, uint8_t pinInput) :
              AbstractSensor(name)
              {
                this->pinInput = pinInput;

                AbstractType::add(
                  new JsonFieldLevel({useType<float>(0.00), "", jsonLevels})
                );
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                float decimalValue = analogRead(pinInput) * 3.333 / 4096,
                      calculatedPressure = (decimalValue - ZERO_PRESSURE_K) * 3.947693,  // Переводим в АТМФ
                      currentValue = calculatedPressure * (RESISTANCE_R1+RESISTANCE_R2) / RESISTANCE_R2;  // Масштабируем за счет делителя

                currentValue = currentValue > 0 ? 0.00 : currentValue;
                setValue(doc, section, currentValue, diff);
              };
        };

      struct PzemSensor : public AbstractSensor
        {
          protected:
            static const uint16_t MAX_AC_VALUE_OR_ERROR = 260,
                                  MAX_CURRENT_VALUE_OR_ERROR = 5000;

          private:
            PZEM004Tv30* sensor;

          public:
            PzemSensor(const char* name, const String& jsonVoltageLevel, const String& jsonCurrentLevel, PZEM004Tv30& sensor) :
              AbstractSensor(name)
              {
                this->sensor = &sensor;

                AbstractType::add(
                  new JsonFieldLevel({useType<uint16_t>(0), "voltage", jsonVoltageLevel}),
                  new JsonFieldLevel({useType<float>(0.00), "current", jsonCurrentLevel})
                );
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                float voltageValue = sensor->voltage();
                if(isnan(voltageValue) || voltageValue > MAX_AC_VALUE_OR_ERROR)
                  setError(doc, section, diff, "voltage");
                else
                  setValue(doc, section, uint16_t(round(voltageValue)), diff, "voltage");

                float currentValue = sensor->current();
                if(isnan(voltageValue) || isnan(currentValue) || currentValue > MAX_CURRENT_VALUE_OR_ERROR)
                  setError(doc, section, diff, "current");
                else
                  setValue(doc, section, currentValue, diff, "current");
              };
        };


      struct BME280WeatherSensor : public AbstractSensor
        {
          protected:
            constexpr static const float PASCALS_IN_MM_HG = 133.3223;  // 1 mmHg = 133.32239 pascals

          private:
            Adafruit_BME280* sensor;

          public:
            BME280WeatherSensor(const char* name, const String& jsonTemperatureLevel, const String& jsonHumidityLevel, const String& jsonPressureLevel, Adafruit_BME280& sensor) :
              AbstractSensor(name)
              {
                this->sensor = &sensor;

                AbstractType::add(
                  new JsonFieldLevel({useType<float>(0.00), "temperature", jsonTemperatureLevel}),
                  new JsonFieldLevel({useType<float>(0.00), "humidity", jsonHumidityLevel}),
                  new JsonFieldLevel({useType<float>(0.00), "pressure", jsonPressureLevel})
                );
              };

            virtual void setup(void)
              {
                sensor->begin();
              }

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                if(!sensor->sensorID())
                  {
                    setAllErrors(doc, section, diff);
                    return;
                  };

                setValue(doc, section, sensor->readTemperature(), diff, "temperature");
                setValue(doc, section, (sensor->readPressure() / PASCALS_IN_MM_HG), diff, "humidity");
                setValue(doc, section, sensor->readHumidity(), diff, "pressure");
              };
        };


    template <typename SerialType> struct UltraSonicSensor : public AbstractSensor
        {
          protected:
            static const int MEASURE_N_TIMES = 3;
            constexpr static const byte REQUEST_CODE = 0x55,
                                        HEADER_CODE = 0xff;

          private:
            SerialType* sensorSerial;

          public:
            UltraSonicSensor(const char* name, const String& jsonLevels, SerialType& espSerial) :
              AbstractSensor(name)
              {
                this->sensorSerial = &espSerial;

                AbstractType::add(
                  new JsonFieldLevel({useType<uint16_t>(0.00), "", jsonLevels})
                );
              };

            virtual void setup(void) override
              {
                sensorSerial->begin(9600);
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                uint16_t totalDistance = 0;
                uint8_t succeededTimes = 0;

                repeat(MEASURE_N_TIMES)
                  {
                    sensorSerial->write(REQUEST_CODE);
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    if(sensorSerial->available())
                      {
                        vTaskDelay(5 / portTICK_PERIOD_MS);
                        if(sensorSerial->read() == HEADER_CODE)
                          {
                            unsigned char buffer[4] = {HEADER_CODE},
                                          checkSum;

                            for(int i = 1; i < 4; i++)
                              buffer[i] = sensorSerial->read();

                            checkSum = buffer[0] + buffer[1] + buffer[2];
                            if(buffer[3] == checkSum)
                              {
                                totalDistance += (buffer[1] << 8) + buffer[2];
                                succeededTimes++;
                              };
                          };
                      };
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                  };

                if(!succeededTimes)
                  setError(doc, section, diff);
                else
                  setValue(doc, section, uint16_t(round(totalDistance / succeededTimes)), diff);
              };
        };
  };
#endif