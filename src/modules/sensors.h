#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <Adafruit_MCP23017.h>
  #include <DallasTemperature.h>
  #include "BaseTypes.h"


  namespace systemSensors
    {
      struct AbstractSensor: public AbstractType
        {
          public:
            const char* name;

            AbstractSensor(const char* name, const String& jsonLevels) :
              AbstractType(jsonLevels)
              {
                this->name = name;
              };
            ~AbstractSensor() {};

            virtual void measure(JsonDocument& doc, const char* section, boolean diff) {};
        };


      template<class RT> struct BaseSensor
        {
          public:
            RT lastValue;

            boolean checkDiffer(RT currentValue, boolean diff)
              {
                if(!diff || (diff && currentValue != lastValue))
                  {
                    lastValue = currentValue;
                    return true;
                  };
                  return false;
              };
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
                        sensor -> measure(*doc, moduleNamespace, diff);
                        return true;
                      };
                  };
                  return false;
              };

            void measureAll(boolean diff=true) {
              for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
                list[sensorIndex] -> measure(*doc, moduleNamespace, diff);
            };
        };


        struct DigitalSensorI2C : private BaseSensor<uint8_t>,
                                  public AbstractSensor
          {
            private:
              Adafruit_MCP23017* board;
              uint8_t pinOutput;

            public:
              DigitalSensorI2C(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, const uint8_t pinOutput) :
                AbstractSensor(name, jsonLevels)
                {
                  this->board = &board;
                  this->pinOutput = pinOutput;
                  this->board->pinMode(pinOutput, INPUT_PULLUP);
                };

              virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
                {
                  uint8_t currentValue = board->digitalRead(pinOutput);  // читаем состоиние на pine
                  checkDiffer(currentValue, diff) && setValue(doc, section, currentValue);
                };
        };


      struct OneWireSensor : private BaseSensor<uint8_t>,
                             public AbstractSensor
        {
          private:
            uint8_t addressIndex;

          public:
            OneWireSensor(const char* name, const String& jsonLevels, const uint8_t addressIndex) :
              AbstractSensor(name, jsonLevels)
              {
                this->addressIndex=addressIndex;
              };

            virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
              {
                uint8_t currentValue = addressIndex;
                checkDiffer(currentValue, diff) && setValue(doc, section, currentValue);
              };
        };

      struct SensorDallasTemperatureOneWire : private BaseSensor<float>,
                                              public AbstractSensor
        {
          static const int ERROR = -127;
          private:
            DeviceAddress* deviceAddress;
            DallasTemperature* controller;

          public:
            SensorDallasTemperatureOneWire(const char* name, const String& levels, DallasTemperature& controller, DeviceAddress& address, uint8_t resolution) :
              AbstractSensor(name, levels)
              {
                this->controller = &controller;
                this->deviceAddress = &address;
                if(!controller.isConnected(address))
                  return;
                controller.setResolution(address, resolution);
              };

            virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
              {
                float currentValue = controller->getTempC(*deviceAddress);
                if(checkDiffer(currentValue, diff))
                  {
                    if(currentValue == ERROR)
                      setValue(doc, section, "error");
                    else
                      setValue(doc, section, currentValue);
                  }
              };
        };

      struct SensorWaterFlow : private BaseSensor<float>,
                               public AbstractSensor
        {
          static const int ERROR = 0;
          private:
            uint8_t pinInput;

          public:
            SensorWaterFlow(const char* name, const String& levels, uint8_t pinInput) :
              AbstractSensor(name, levels)
              {
                this->pinInput = pinInput;
                pinMode(pinInput, INPUT);
              };

            virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
              {
                float totalPulse = pulseIn(pinInput, HIGH) + pulseIn(pinInput, LOW);
                if(!totalPulse)
                  {
                    setValue(doc, section, "error");
                    return;
                  };
                uint16_t currentValue = round(1000000 / totalPulse);
                checkDiffer(currentValue, diff) && setValue(doc, section, currentValue);
              };
        };

      struct SensorWaterPressure : private BaseSensor<float>,
                                   public AbstractSensor
        {          
          private:            
            static const uint8_t zeroPressureK = 0.17;
            static const uint16_t resistanceR1 = 1000,  // делитель 5v -> 3.3v
                                  resistanceR2 = 2000;
            uint8_t pinInput;

          public:
            SensorWaterPressure(const char* name, const String& levels, uint8_t pinInput) :
              AbstractSensor(name, levels)
              {
                this->pinInput = pinInput;                
              };

            virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
              {                
                float decimalValue = analogRead(pinInput) * 3.333 / 4096,
                      calculatedPressure = (decimalValue - zeroPressureK) * 3.947693,  // Переводим в АТМФ
                      currentValue = calculatedPressure * (resistanceR1+resistanceR2) / resistanceR2;  // Масштабируем за счет делителя

                currentValue = currentValue > 0 ? 0.00 : currentValue;
                checkDiffer(currentValue, diff) && setValue(doc, section, currentValue);
              };
        };
    };
#endif