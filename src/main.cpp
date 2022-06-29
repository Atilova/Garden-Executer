#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include <modules/sensors.h>

#define len(object) sizeof(object) / sizeof(*object)


Adafruit_MCP23017 sensorsBoard;

BaseSensor* sensorsList[] = {
  new SensorI2C {"SENSOR1", sensorsBoard, 5},
  new OneWireSensor {"SENSOR2", 1},
  new OneWireSensor {"SENSOR4", 3}
};

Sensors sensors(sensorsList, len(sensorsList));

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  sensors.get("SENSOR1");
  sensors.get("SENSOR2");
};

void loop() {

};