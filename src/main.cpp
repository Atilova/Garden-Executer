#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include <modules/Init.h>
#include <ArduinoJson.h>
#include <Settings.h>
#include <Secrete.h>
#include <modules/Mqtt.h>
#include <Esp.h>




mosquitto::MqttConsumer mqttEngine;

StaticJsonDocument<500> doc;
Adafruit_MCP23017 powerBoardI2C,
                  sensorsBoardI2C;

RelayController relays;
SensorsController sensors;

Relay relaysList[] =
  {
    Relay {"SYSTEM_5V_RELAY", "5v", powerBoardI2C, 0},
    Relay {"SYSTEM_12V_RELAY", "12v", powerBoardI2C, 1},
    Relay {"SYSTEM_24V_RELAY", "24v", powerBoardI2C, 2},
    Relay {"SYSTEM_CONTACTOR", "contactor", powerBoardI2C, 3},
    Relay {"SYSTEM_WELL_PUMP", "wellPump", powerBoardI2C, 4}
  };

AbstractSensor* sensorsList[] =
  {
    new SensorI2C {"SENSOR1", "tank.level.test", sensorsBoardI2C, 5},
    new SensorI2C {"SENSOR2", "feedback.ac", sensorsBoardI2C, 2},
    new OneWireSensor {"SENSOR3", "weather.humidity", 1}
  };


void setup()
  {
    Serial.begin(115200);
    Serial.println();

    Wire.begin();    
    powerBoardI2C.writeGPIOAB(0xfff);
    powerBoardI2C.begin(0x0);  // плата управления силовыми реле сист. полива
    sensorsBoardI2C.begin(0x2);  // сигналы от датчиков обратной связи
    
    relays.configure(relaysList, len(relaysList), "relays", doc);
    sensors.configure(sensorsList, len(sensorsList), "sensors", doc);

    mqttEngine.configure(mqttConfig);


  //   delay(1000);
  //   relays.setState("SYSTEM_5V_RELAY", true);
  //   relays.setState("SYSTEM_CONTACTOR", true);
  //   relays.setState("SYSTEM_12V_RELAY", true);

  //   sensors.measureAll();
  //   relays.readAll();
  //   serializeJsonPretty(doc, Serial);

    mqttEngine.start();
  };

void loop()
  {
   
  };