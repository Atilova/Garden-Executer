#ifndef _SETTINGS_H_
  #define _SETTINGS_H_
  
  #include <Arduino.h>
  #include "Secrete.h"


  struct ProjectConfig : Secrete
    {
      IPAddress WIFI_IP = IPAddress(192, 168, 1, 111);
      IPAddress WIFI_GATEWAY = IPAddress(192, 168, 1, 1);
      IPAddress WIFI_SUBNET_MASK = IPAddress(255, 255, 255, 0);
      IPAddress WIFI_PRIMARY_DNS = IPAddress(192, 168, 1, 1);
      IPAddress WIFI_SECONDARY_DNS = IPAddress(8, 8, 8, 8);    
    };


  // #include <modules/Mqtt.h>

  // #define len(object) sizeof(object) / sizeof(*object)


  // const char* topicsSubscribe[] = 
  //   {
  //     "garden/tester",
  //     "garden/alarm"
  //   };

  // mosquitto::Config mqttConfig = 
  //   {
  //     .listeners=topicsSubscribe,
  //     .listenersSize=len(topicsSubscribe),
  //     .host="192.168.1.13",
  //     .port=1883,
  //     .keepAlive=10,
  //     .socketTimeout=12,
  //     .clientId="EXECUTER_HOST",
  //     .mqttUsername=MQTT_USERNAME,
  //     .mqttPassword=MQTT_PASSWORD,
  //     .wifiPoint="Tenda_62176",
  //     // .wifiPoint="we..",
  //     .wifiPassword=WIFI_PASSWORD
  //   };

#endif