#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_BME280.h>
#include <modules/Init.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

#define len(object) sizeof(object) / sizeof(*object)

#define ONE_WIRE_BUS 17

StaticJsonDocument<500> doc;
RelayController relays;
SensorsController sensors;

Adafruit_MCP23017 powerBoardI2C,
                  sensorsBoardI2C;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemperatureSensors(&oneWire);
DeviceAddress powerBoxThermometer = {0x28, 0xC, 0x1, 0x7, 0x2D, 0xDA, 0x1, 0x4D}, 
              videoBoxThermometer = {0x28, 0x9B, 0x3, 0xB1, 0x2F, 0x14, 0x1, 0x45};  // On the long wire

// PZEM004Tv30 pzemSensor(Serial2, 15, 16);
PZEM004Tv30 pzemSensor;
Adafruit_BME280 weatherSensor;
SoftwareSerial ultraSonicSensorSerial(14, 27);

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
    new DallasTemperatureOneWireSensor {"TEMPERATURE1", "temperature.power", dallasTemperatureSensors, powerBoxThermometer, 9},
    new DallasTemperatureOneWireSensor {"TEMPERATURE2", "temperature.video", dallasTemperatureSensors, videoBoxThermometer, 9},
    new WaterFlowSensor {"WELL_FLOW", "pumps.wellFlow", 34},
    new WaterPressureSensor {"WATER_PRESSURE", "pumps.gardenPressure", 32},
    new PzemSensor {"PZEM_VOLTAGE", "voltage.ac", "pumps.wellCurrent", pzemSensor},
    new BME280WeatherSensor {"WEATHER_SENSOR", "weather.temperature", "weather.humidity", "weather.pressure", weatherSensor},
    new UltraSonicSensor<SoftwareSerial> {"WATER_LEVEL_SENSOR", "tank.level.sensor", ultraSonicSensorSerial}
  };

void measure()
  {
    sensors.measureAll();
    if(!doc.isNull())
      {
        serializeJsonPretty(doc, Serial);
        doc.clear();
      };    
  };

void setup()
  {
    Serial.begin(115200);
    Serial.println();

    Wire.begin();
    powerBoardI2C.writeGPIOAB(0xfff);
    powerBoardI2C.begin(0x0);  // Плата управления силовыми реле сист. полива
    sensorsBoardI2C.begin(0x2);  // Сигналы от датчиков обратной связи    
    relays.configure(relaysList, len(relaysList), "relays", doc);
    sensors.configure(sensorsList, len(sensorsList), "sensors", doc);  
  };

void loop()
  {
    measure();
    delay(2000);
  };