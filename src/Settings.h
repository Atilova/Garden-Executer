#ifndef _SETTINGS_H_
  #define _SETTINGS_H_

  #include <IPAddress.h>
  #include "Secrete.h"


  struct ProjectConfig : Secrete
    {
      IPAddress WIFI_IP = IPAddress(192, 168, 1, 111);
      IPAddress WIFI_GATEWAY = IPAddress(192, 168, 1, 1);
      IPAddress WIFI_SUBNET_MASK = IPAddress(255, 255, 255, 0);
      IPAddress WIFI_PRIMARY_DNS = IPAddress(192, 168, 1, 1);
      IPAddress WIFI_SECONDARY_DNS = IPAddress(8, 8, 8, 8);
      char DATABASE_NAME[30] = "statistics";
      IPAddress DATABASE_HOST = IPAddress(192, 168, 1, 113);
      const uint16_t DATABASE_PORT = 3306;
    };
#endif