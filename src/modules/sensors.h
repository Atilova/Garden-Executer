#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <Adafruit_MCP23017.h>
  #include <ArduinoJson.h>


  struct AbstractSensor
    {
      private:
        const char* docLevel;

      public:
        const char* name;

        AbstractSensor(const char* name, const char* level)
          {
            this->name = name;
            docLevel = level;
          };
        ~AbstractSensor() {};

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) {};
    };


  template<class RT> struct BaseSetup
    {
      private:
        RT lastValue;
    };


  struct SensorI2C : private BaseSetup<uint8_t>,
                     public AbstractSensor
    {
      private:
        Adafruit_MCP23017 board;
        uint8_t pinOutput;

      public:
        SensorI2C(const char* name, const char* level, Adafruit_MCP23017 board, const uint8_t pinOutput): AbstractSensor(name, level)
          {
            this->board = board;
            this->pinOutput=pinOutput;
          };

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true)
          {
            Serial.print("Measure I2c -> ");
            doc["q"] = 2;
            Serial.println(pinOutput);
          };
  };


  struct OneWireSensor : private BaseSetup<uint8_t>,
                         public AbstractSensor
    {
      private:
        uint8_t addressIndex;

      public:
        OneWireSensor(const char* name, const char* level, const uint8_t addressIndex): AbstractSensor(name, level)
          {
            this->addressIndex=addressIndex;
          };

        virtual void measure(JsonDocument& doc, const char* section, boolean diff=true) override
          {
            Serial.print("Measure OneWire -> ");
            doc["w"] = 1;
            Serial.println(addressIndex);
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
            listSize = size;
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