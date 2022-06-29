#ifndef _SENSORS_H_
  #define _SENSORS_H_

  #include <Arduino.h>
  #include <Adafruit_MCP23017.h>


  struct BaseSensor {
    public:
      String name;
      BaseSensor(String name) {
        this->name = name;
      };
      ~BaseSensor() {};
      virtual int measure(void) {return 0;};
  };


  struct SensorI2C : public BaseSensor {
    private:
      Adafruit_MCP23017 board;
      uint8_t pinOutput;

    public:
      SensorI2C(String name, Adafruit_MCP23017 board, uint8_t pinOutput): BaseSensor(name) {
        this->board = board;
        this->pinOutput=pinOutput;
      };

      virtual int measure(void) {
        Serial.print("Measure I2c -> ");
        Serial.println(pinOutput);
        return 20;
      };
  };


  struct OneWireSensor : public BaseSensor {    
    private:
      uint8_t addressIndex;

    public:  
      OneWireSensor(String name, uint8_t addressIndex): BaseSensor(name) {
        this->addressIndex=addressIndex;
      };

      virtual int measure(void) {
        Serial.print("Measure OneWire -> ");
        Serial.println(addressIndex);
        return 10;
      };
  };

  
  class Sensors {
    private:
      BaseSensor** list;
      uint8_t listSize;
    
    public:
      Sensors(BaseSensor** list, uint8_t size) {
        this->list = list;
        listSize = size;
      };
      ~Sensors(void) {};

      void get(const char* name) {
        for(uint8_t sensorIndex=0; sensorIndex<listSize; sensorIndex++) {
          BaseSensor* sensor = list[sensorIndex];
          if(sensor->name == name) {
            sensor->measure();
            return;  
          };
        };
      };
  };
#endif