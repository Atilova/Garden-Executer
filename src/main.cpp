#include <Arduino.h>
#include <Server.h>
#include <SparkFun_AS3935.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <Lightning.h>

#define SPARK_SENSOR_INTERRUPT_PIN 5
#define SPARK_SENSOR_AS3935_ADDRESS 0x03

StaticJsonDocument<500> doc;

SparkFun_AS3935 sparkSensor(SPARK_SENSOR_AS3935_ADDRESS);
SparkSensorController smartSparkController(sparkSensor, 48, SPARK_SENSOR_INTERRUPT_PIN);
WebsocketAPIServer wsServer(smartSparkController);


void setup()
  {
    Serial.begin(115200);
    Serial.println();

    Wire.begin();
    EEPROM.begin(512);
    SPIFFS.begin();

    smartSparkController.init();

    wsServer.run();
  };

void loop()
 {
  smartSparkController.loop();
 };