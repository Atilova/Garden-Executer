#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include <modules/sensors.h>
#include <ArduinoJson.h>
#include <Esp.h>

#define len(object) sizeof(object) / sizeof(*object)


StaticJsonDocument<500> doc;

Adafruit_MCP23017 sensorsBoard;
AbstractSensor* sensorsList[] = {
  new SensorI2C {"SENSOR1", "tank.level.test", sensorsBoard, 5},  
  new SensorI2C {"SENSOR2", "feedback.ac", sensorsBoard, 2},  
  new OneWireSensor {"SENSOR3", "weather.humidity", 1},  
};

Sensors sensors(sensorsList, len(sensorsList), "sensors", doc);



void measure() {
  Serial.println("Measuring...");
  Serial.println(ESP.getFreeHeap());
  sensors.measure("SENSOR1");
  Serial.println(ESP.getFreeHeap());
  sensors.measure("SENSOR2");
  Serial.println(ESP.getFreeHeap());
  sensors.measure("SENSOR3");
  Serial.println(ESP.getFreeHeap());
  sensors.measure("SENSOR4");
  Serial.println(ESP.getFreeHeap());
  serializeJsonPretty(doc, Serial);
}

void setup() 
  {
    Serial.begin(115200);
    Serial.println();
    delay(1000);

    // measure();
  };



void loop() 
  {
    measure();
    delay(300);
  };