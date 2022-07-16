#include <Arduino.h>
#include "SparkFun_AS3935.h"
#include <Server.h>
#include "SparkFun_AS3935.h"
#include <Wire.h>
#include "SPIFFS.h"

#define SPARK_SENSOR_INTERRUPT_PIN 5

StaticJsonDocument<500> doc;
WebsocketAPIServer wsServer;

SparkFun_AS3935 sparkSensor(INDOOR);

uint8_t noise = 7; // Value between 1-7 было 2
uint8_t disturber = 6; // Value between 1-10 ,default -2
// uint8_t spike = 6;  // def=2 
uint8_t lightningThresh = 1;  //def=1, количество учитываемых вспышек молний в 15-ми минутном интервале до подачи прерывания IRq

volatile bool sparkHasDetected;

void sparkUpdateISR()
  {
    sparkHasDetected = true;
  };

void setup()
  {
    Serial.begin(115200);
    Serial.println();

    if(!SPIFFS.begin(true)) {
      Serial.println("An Error has occurred while mounting SPIFFS");
    };

    // Wire.begin();
    // pinMode(SPARK_SENSOR_INTERRUPT_PIN, INPUT_PULLUP);

    // sparkSensor.begin();
    // sparkSensor.tuneCap(48);  // Установка значения конденсатора для нашей платы
    // sparkSensor.calibrateOsc();

    // sparkSensor.setNoiseLevel(noise);
    // sparkSensor.maskDisturber(false);  // Инфо о наличие помех ВКЛ - false
    // sparkSensor.watchdogThreshold(disturber);
    // sparkSensor.lightningThreshold(lightningThresh); // Запись в модуль количества учитываемых молний за 15 мин
    // sparkSensor.spikeRejection(spike); 


    // attachInterrupt(SPARK_SENSOR_INTERRUPT_PIN, sparkUpdateISR, HIGH);

    wsServer.run();
    
  };


void checkSparkResponse()
  {

  }

void loop()
 {
    doc["value"] = random(100);
    wsServer.notifyAll(doc);
    delay(5000);   
 };


  // int sparkResponse = sparkSensor.readInterruptReg();
  // switch(sparkResponse)
  //   {
  //     case NOISE_TO_HIGH:
  //       {
  //         Serial.println("Spark -> NOISE (Шум)");
  //         break;
  //       };
  //     case DISTURBER_DETECT:
  //       {
  //         Serial.println("Spark -> DISTURBER (Помеха)");          
  //         break;
  //       };
  //     case LIGHTNING:
  //       {
  //         uint8_t lightningDistance = sparkSensor.distanceToStorm(); 
  //         uint32_t lightningEnergy = sparkSensor.lightningEnergy(); 

  //         Serial.println("Spark -> LIGHTNING ()");
  //         break;
  //       };
  //     default:
  //       {
  //         Serial.println("Spark -> UNTRACKED");
  //         break;
  //       };
  //   }