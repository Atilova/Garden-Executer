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
    // new DigitalSensorI2C {"SENSOR1", "tank.level.top", sensorsBoardI2C, 1},
    // new SensorDallasTemperatureOneWire {"TEMPERATURE1", "temperature.power", dallasTemperatureSensors, powerBoxThermometer, 9},
    // new SensorWaterFlow {"WELL_FLOW", "pumps.wellFlow", 34},
    // new SensorWaterPressure {"WATER_PRESSURE", "pumps.gardenPressure", 32},
  };


void setup()
  {
    Serial.begin(115200);
    Serial.println();

    // Wire.begin();
    // powerBoardI2C.writeGPIOAB(0xfff);
    // powerBoardI2C.begin(0x0);  // Плата управления силовыми реле сист. полива
    // sensorsBoardI2C.begin(0x2);  // Сигналы от датчиков обратной связи
    // dallasTemperatureSensors.begin();
    // relays.configure(relaysList, len(relaysList), "relays", doc);
    // sensors.configure(sensorsList, len(sensorsList), "sensors", doc);

    delay(1000);
    PzemSensor pzemX {"PZEM_VOLTAGE", "voltage.stabilizer.ac", "pumps.wellCurrent", pzemSensor};
    // pzemX.measure(doc, "sensors", false);
    // serializeJsonPretty(doc, Serial);
  };

void loop()
  {

  };

  
// template<typename T> T* var(T defaultVal) {
//   T* ptr = &defaultVal;
//   Serial.println(*ptr);
//   T* val =  static_cast<T*>(ptr);
//   Serial.println(*val);
//   return val;
// };
// template<typename T> T* var(T defaultVal) {
  // T* ptr = &defaultVal;
  // return ptr;
  // Serial.println(*ptr);
  // T* val =  static_cast<T*>(ptr);
  // Serial.println(*val);
  // return val;
// };

// class C 
//   {
//     template<class T> C(T*)
//       {

//       };
//   };

// template<typename T> T* var() {
//   return static_cast<T*>(nullptr);
// };

// class C
//   {
//     public:
//       template<typename T> C(T* s) {
//         Serial.print("Q -> ");

//         T x = *s;
//         Serial.println(x);
//       };
//   };

// String* a = var<String>("hey");
// Serial.println(*a);

// new C {var<String>("hey")};
// new C {var<int>(10)};

// new C {var<boolean>()};
// new C {var<float>()};
// new C {var<char>()};
// new C {var<char*>()};
  // };

  // template<class...>struct types{using type=types;};
  // auto (*TOKEN)(void)
// auto qw = std::bind(&ReturnType<String>::caller, x);
// auto* anotherX = qw();
// Serial.println(anotherX->lastType);