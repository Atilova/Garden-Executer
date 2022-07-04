#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include <modules/Init.h>
#include <ArduinoJson.h>
#include <Esp.h>

#define len(object) sizeof(object) / sizeof(*object)


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
    new DigitalSensorI2C {"SENSOR1", "tank.level.top", sensorsBoardI2C, 1},
    new DigitalSensorI2C {"SENSOR2", "tank.level.middle", sensorsBoardI2C, 2},
    new DigitalSensorI2C {"SENSOR3", "tank.level.low", sensorsBoardI2C, 3},
    new DigitalSensorI2C {"SENSOR4", "tank.level.bottom", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK1", "feedback.ac", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK2", "feedback.5v", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK3", "feedback.12v", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK4", "feedback.24v", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK5", "feedback.contactor", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK6", "feedback.stabilizer", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK7", "feedback.wellPump", sensorsBoardI2C, 4},
    new DigitalSensorI2C {"FEEDBACK8", "feedback.gardenPum", sensorsBoardI2C, 4},
    
    // new SensorI2C {"SENSOR2", "feedback.ac", sensorsBoardI2C, 2},
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


    delay(1000);
    relays.setState("SYSTEM_5V_RELAY", true);
    relays.setState("SYSTEM_CONTACTOR", true);
    relays.setState("SYSTEM_12V_RELAY", true);

    // sensors.measure("SENSOR1", ); // прочитать значение только одного датчика
    sensors.measureAll(false);  // true - создаем json только измененных значений, false - будет создана таблица всех значений
    relays.readAll();
    serializeJsonPretty(doc, Serial);   
  };

void loop()
  {
   
  };