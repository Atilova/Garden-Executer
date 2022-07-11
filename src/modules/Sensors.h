#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <Adafruit_MCP23017.h>
  #include <DallasTemperature.h>
  #include <PZEM004Tv30.h>
  #include "BaseTypes.h"


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
              };

            boolean measure(const char* name, boolean diff=true)
              {
                for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                  {
                    AbstractSensor* sensor = list[sensorIndex];
                    if(sensor->name == name)
                      {
                        sensor -> measure(*doc, diff, moduleNamespace);
                        return true;
                      };
                  };
                  return false;
              };

            void measureAll(boolean diff=true) {
              for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                list[sensorIndex] -> measure(*doc, diff, moduleNamespace);
            };
        };


      struct DigitalSensorI2C : public AbstractSensor
        {
          private:
            Adafruit_MCP23017* board;
            uint8_t pinOutput;

          public:
            DigitalSensorI2C(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, const uint8_t pinOutput) :
              AbstractSensor(name)
              {
                this->board = &board;
                this->pinOutput = pinOutput;
                this->board->pinMode(pinOutput, INPUT_PULLUP);

                AbstractType::add(
                  new JsonFieldLevel({useType<boolean>(false), "", jsonLevels})                  
                );
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                uint8_t currentValue = board->digitalRead(pinOutput);  // читаем состоиние на pine
                setValue(doc, section, currentValue, diff);
              };
        };


      struct SensorDallasTemperatureOneWire : public AbstractSensor
        {
          static const int ERROR = -127;
          private:
            DeviceAddress* deviceAddress;
            DallasTemperature* controller;

          public:
            SensorDallasTemperatureOneWire(const char* name, const String& jsonLevels, DallasTemperature& controller, DeviceAddress& address, uint8_t resolution) :
              AbstractSensor(name)
              {
                this->controller = &controller;
                this->deviceAddress = &address;
                if(!controller.isConnected(address))
                  return;

                controller.setResolution(address, resolution);
                AbstractType::add(
                  new JsonFieldLevel({useType<float>(0.00), "", jsonLevels})                  
                );
              };

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                float currentValue = controller->getTempC(*deviceAddress);                
                currentValue == ERROR ? setError(doc, section, diff) : setValue(doc, section, currentValue, diff);
              };
        };


      struct SensorWaterFlow : public AbstractSensor
        {
          static const int ERROR = 0;
          private:
            uint8_t pinInput;

          public:
            SensorWaterFlow(const char* name, const String& jsonLevels, uint8_t pinInput) :
              AbstractSensor(name)
              {
                this->pinInput = pinInput;
                pinMode(pinInput, INPUT);

                AbstractType::add(
                  new JsonFieldLevel({useType<uint16_t>(0), "", jsonLevels})                  
                );
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


      struct SensorWaterPressure : public AbstractSensor
        {
          private:
            static const uint8_t zeroPressureK = 0.17;
            static const uint16_t resistanceR1 = 1000,  // делитель 5v -> 3.3v
                                  resistanceR2 = 2000;
            uint8_t pinInput;

          public:
            SensorWaterPressure(const char* name, const String& jsonLevels, uint8_t pinInput) :
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
                      calculatedPressure = (decimalValue - zeroPressureK) * 3.947693,  // Переводим в АТМФ
                      currentValue = calculatedPressure * (resistanceR1+resistanceR2) / resistanceR2;  // Масштабируем за счет делителя

                currentValue = currentValue > 0 ? 0.00 : currentValue;
                setValue(doc, section, currentValue, diff);
              };
        };
      
      struct PzemSensor : public AbstractSensor
        {
          private:
            const char* name;
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
            ~PzemSensor() {};

            virtual void measure(JsonDocument& doc, boolean diff, const char* section) override
              {
                float voltageValue = sensor->voltage();
                if(isnan(voltageValue))
                  setError(doc, section, diff, "voltage");
                else
                  setValue(doc, section, uint16_t(round(voltageValue)), diff, "voltage");

                float currentValue = sensor->current();
                if(isnan(currentValue))
                  setError(doc, section, diff, "current");
                else
                  setValue(doc, section, currentValue, diff, "current");
              };
        };
  };
#endif