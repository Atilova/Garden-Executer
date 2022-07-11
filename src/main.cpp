#include <Arduino.h>
#include <Adafruit_MCP23017.h>
// #include <Adafruit_BME280.h>
#include <modules/Init.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PZEM004Tv30.h>

#define len(object) sizeof(object) / sizeof(*object)

#define ONE_WIRE_BUS 48  // Подключение цифрового вывода датчика температуры к 48 пину +4,7 на+5в


StaticJsonDocument<500> doc;
RelayController relays;
SensorsController sensors;

OneWire oneWire(ONE_WIRE_BUS);  // Запуск интерфейса OneWire для подключения OneWire устройств.
DallasTemperature dallasTemperatureSensors(&oneWire);  // Указание, что устройством oneWire является термодатчик от  Dallas Temperature.
DeviceAddress powerBoxThermometer = {0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0};
PZEM004Tv30 pzemSensor(&Serial2);
// Adafruit_BME280 weatherSensor;

Adafruit_MCP23017 powerBoardI2C,
                  sensorsBoardI2C;

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
    new SensorDallasTemperatureOneWire {"TEMPERATURE1", "temperature.power", dallasTemperatureSensors, powerBoxThermometer, 9},
    new SensorWaterFlow {"WELL_FLOW", "pumps.wellFlow", 34},
    new SensorWaterPressure {"WATER_PRESSURE", "pumps.gardenPressure", 32},
    new PzemSensor {"PZEM_VOLTAGE", "voltage.ac", "pumps.wellCurrent", pzemSensor}
  };

void setup()
  {
    Serial.begin(115200);
    Serial.println();
    delay(1000);    

    Wire.begin();
    powerBoardI2C.writeGPIOAB(0xfff);
    powerBoardI2C.begin(0x0);  // Плата управления силовыми реле сист. полива
    sensorsBoardI2C.begin(0x2);  // Сигналы от датчиков обратной связи
    dallasTemperatureSensors.begin();
    relays.configure(relaysList, len(relaysList), "relays", doc);
    sensors.configure(sensorsList, len(sensorsList), "sensors", doc);
  };

void loop()
  {
    sensors.measureAll();
    if(!doc.isNull())
      {
        serializeJsonPretty(doc, Serial);
        doc.clear();
      };
    delay(2000);
  };