#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <Adafruit_MCP23017.h>
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


        struct SensorI2C : private BaseSensor<uint8_t>,
                           public AbstractSensor
          {
            private:
              Adafruit_MCP23017* board;
              uint8_t pinOutput;

            public:
              SensorI2C(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, const uint8_t pinOutput) :
                AbstractSensor(name, jsonLevels)
                {
                  this->board = &board;
                  this->pinOutput = pinOutput;
                  this->board->pinMode(pinOutput, INPUT_PULLUP);
                };

              virtual void measure(JsonDocument& doc, const char* section, boolean diff) override
                {
                  // Serial.print("Measure I2c -> ");
                  uint8_t currentValue = pinOutput;
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
                // Serial.print("Measure OneWire -> ");
                uint8_t currentValue = addressIndex;
                checkDiffer(currentValue, diff) && setValue(doc, section, currentValue);
              };
        };
    };

    // struct SensorTemplate : private BaseSensor<>,
    //                         public AbstractSensor
    //   {
    //     public:
    //       SensorTemplate(const char* name, const String& levels) : AbstractSensor(name, levels)
    //         {

    //         };

    //       virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) override
    //         {
    //           checkDiffer(currentValue, diff) && JsonWorkflow::setValue(doc, section, splittedLevels, levelNestedSize, currentValue);
    //         };
    //   };
#endif