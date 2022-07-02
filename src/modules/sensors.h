#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <Adafruit_MCP23017.h>
  #include <ArduinoJson.h>
  #include "JsonFlow.h"


  struct AbstractSensor
    {
      public:
        const char* name;      
        uint8_t levelNestedSize = 1;
        char** splittedLevels;
        
        AbstractSensor(const char* name, const String& levels)
          {
            this->name = name;
            this->levelNestedSize += std::count(levels.begin(), levels.end(), '.');
            this->splittedLevels = new char* [levelNestedSize];

            char levelCharArray[levels.length()];
            strcpy(levelCharArray, levels.c_str());

            char* token = strtok(levelCharArray, ".");
            uint8_t index = 0;

            while(token)
              {
                splittedLevels[index] = new char[strlen(token)+1];
                strcpy(splittedLevels[index], token);
                token = strtok(NULL, ".");
                index++;
              };
          };
        ~AbstractSensor() {};

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) {};
    };


  template<class RT> struct BaseSetup
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


  struct SensorI2C : private BaseSetup<uint8_t>,
                     public AbstractSensor
    {
      private:
        Adafruit_MCP23017* board;
        uint8_t pinOutput;

      public:
        SensorI2C(const char* name, const String& levels, Adafruit_MCP23017& board, const uint8_t pinOutput) : AbstractSensor(name, levels)
          {
            this->board = &board;
            this->pinOutput = pinOutput;
            this->board->pinMode(pinOutput, INPUT_PULLUP);
          };

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) override
          {
            Serial.print("Measure I2c -> ");
            uint8_t currentValue = 10;
            checkDiffer(currentValue, diff) && JsonWorkflow::setValue(doc, section, splittedLevels, levelNestedSize, currentValue);
          };
  };


  struct OneWireSensor : public BaseSetup<uint8_t>,
                         public AbstractSensor
    {
      private:
        uint8_t addressIndex;

      public:
        OneWireSensor(const char* name, const String& levels, const uint8_t addressIndex): AbstractSensor(name, levels)
          {
            this->addressIndex=addressIndex;
          };

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) override
          {
            Serial.print("Measure OneWire -> ");            
            uint8_t currentValue = addressIndex;
            checkDiffer(currentValue, diff) && JsonWorkflow::setValue(doc, section, splittedLevels, levelNestedSize, currentValue);
          };
    };


  class Sensors
    {
      private:
        AbstractSensor** list;
        uint8_t listSize;
        const char* moduleNamespace;
        JsonDocument* doc;

      public:
        Sensors(AbstractSensor** list, uint8_t size, const char* moduleNamespace, JsonDocument& doc)
          {
            this->list = list;
            this->listSize = size;
            this->moduleNamespace = moduleNamespace;
            this->doc = &doc;
          };
        ~Sensors(void) {};

        boolean measure(const char* name)
          {
            for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++)
              {
                AbstractSensor* sensor = list[sensorIndex];
                if(sensor->name == name)
                  {
                    sensor -> measure(*doc, moduleNamespace);
                    return true;
                  };
              };
              return false;
          };
    };
#endif