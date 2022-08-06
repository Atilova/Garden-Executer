#ifndef _SETTINGS_H_
  #define _SETTINGS_H_

  #include <Arduino.h>
  #include "Secrete.h"
  #include "Functions.h"
  #include <modules/Mqtt.h>


  struct ProjectConfig : Secrete
    {

    };

  ProjectConfig globalConfig;


  const char* TOPICS_LISTEN[] =
    {
      "tester/extra/listener"
    };

  Mqtt::Config mqttConfig =
    {
      .WIFI_IP = IPAddress(192, 168, 1, 111),
      .WIFI_GATEWAY = IPAddress(192, 168, 1, 1),
      .WIFI_SUBNET_MASK = IPAddress(255, 255, 255, 0),
      .WIFI_PRIMARY_DNS = IPAddress(192, 168, 1, 1),
      .WIFI_SECONDARY_DNS = IPAddress(8, 8, 8, 8),
      .WIFI_SSID = globalConfig.WIFI_SSID,
      .WIFI_PASSWORD = globalConfig.WIFI_PASSWORD,
      .SERVER_TOPIC_LISTEN = "tester/listen",
      .SERVER_TOPIC_PUBLISH = "tester/publish",
      .LISTENERS   = TOPICS_LISTEN,
      .LISTENERS_SIZE   = len(TOPICS_LISTEN),
      .HOST = "192.168.1.113",
      .PORT = 1883,
      .KEEPALIVE = 10,
      .SOCKET_TIMEOUT = 12,
      .CLIENT_ID = "EXECUTER_HOST",
      .MQTT_USERNAME = globalConfig.MQTT_USERNAME,
      .MQTT_PASSWORD = globalConfig.MQTT_PASSWORD,
    };
#endif