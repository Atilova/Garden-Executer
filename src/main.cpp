#include <Arduino.h>
#include <EEPROM.h>

#define SPARK_SENSOR_AS3935_ADDRESS 0x03
#define EEPROM_CONFIG_ADDRESS 0x0
#define BME280_WEATHER_SENSOR_ADDRESS 0x76

// #include <LightningMonitoring.h>

// using namespace lightningMonitoring;

// LightningMonitor wsServerMonitor(48, 18);


#include <WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <Settings.h>


ProjectConfig conf;
WiFiClient espWifiClient;
MySQL_Connection conn((Client*)&espWifiClient);

char query[] = "SELECT content FROM temp";
MySQL_Cursor cursor = MySQL_Cursor(&conn);


void setup()
  {
    Serial.begin(115200);
    Serial.println();
    EEPROM.begin(512);

    WiFi.config(conf.WIFI_IP, conf.WIFI_GATEWAY, conf.WIFI_SUBNET_MASK, conf.WIFI_PRIMARY_DNS, conf.WIFI_SECONDARY_DNS);
    WiFi.begin(conf.WIFI_SSID, conf.WIFI_PASSWORD);

    while(!WiFi.isConnected()) {
      Serial.println("ERROR");
      delay(1000);
    }

    Serial.print("WiFI Connected -> ");
    Serial.println(WiFi.localIP());

    if(conn.connect(conf.DATABASE_HOST, conf.DATABASE_PORT, conf.DATABASE_USER, conf.DATABASE_PASSWORD, conf.DATABASE_NAME))
      {
        Serial.println("Done");
        cursor.execute(query);
        cursor.get_columns();

        row_values* row = NULL;
        do {
          row = cursor.get_next_row();
          if (row != NULL) {
            Serial.println(row->values[0]);
          }
        } while (row != NULL);

        cursor.close();
        conn.close();
      };


    // wsServerMonitor.init();
    // wsServerMonitor.run();
  };

void loop()
  {
    // wsServerMonitor.loop();
  };