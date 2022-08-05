#include <Arduino.h>
#include <EEPROM.h>
#include <time.h>

#define SPARK_SENSOR_AS3935_ADDRESS 0x03
#define EEPROM_CONFIG_ADDRESS 0x0
#define BME280_WEATHER_SENSOR_ADDRESS 0x76

// #include <LightningMonitoring.h>

// using namespace lightningMonitoring;

// LightningMonitor wsServerMonitor(48, 18);


#include <WiFi.h>
// #include <WiFiMulti.h>
// #include <MySQL_Connection.h>
// #include <MySQL_Cursor.h>
#include <Settings.h>

// #include <CronAlarms.h>

// #include <ESPAsyncWebServer.h>
// #include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>


// constexpr uint16_t PORT = 80;
// AsyncWebServer server = AsyncWebServer(PORT);
// AsyncWebSocket ws = AsyncWebSocket("/ws/");


// void onWsEventUpdate(AsyncWebSocket* socket, AsyncWebSocketClient* client, AwsEventType wsType, void* arg, uint8_t* data, size_t len) {
//   Serial.println("NEW EVENT");
// };


// // "SELECT * FROM locator WHERE report_for >= DATE_SUB(NOW(), INTERVAL 1 HOUR)";

ProjectConfig conf;
// WiFiClient espWifiClient;
// MySQL_Connection conn((Client*)&espWifiClient);

// constexpr char TIME_FORMAT[] = "%d-%02d-%02d %02d:%02d:%02d",
//                INSERT_LOCATOR_QUERY_TEMPLATE[] = "INSERT INTO locator (report_for, noises, disturbers) VALUES ('%s', %d, %d)",
//                SELECT_LOCATOR_QUERY[] = "SELECT report_for, noises, disturbers FROM locator WHERE report_for >= DATE_SUB(NOW(), INTERVAL 1 HOUR) ORDER BY id DESC LIMIT 60;";


// WiFiMulti wifiMulti;

void connectToWifi() {
  WiFi.begin(conf.WIFI_SSID, conf.WIFI_PASSWORD);

  while(!WiFi.isConnected()) {
    Serial.println("ERROR");
    delay(1000);
  };
};

// boolean makeDatabaseConnection() {  
//   if(conn.connected())
//     return true;
  
//   if(!WiFi.isConnected())
//     connectToWifi();
  
//   return conn.connect(conf.DATABASE_HOST, conf.DATABASE_PORT, conf.DATABASE_USER, conf.DATABASE_PASSWORD, conf.DATABASE_NAME);    
// };

// void startWebServer() {
//   ws.onEvent(onWsEventUpdate);
//   server.addHandler(&ws);

//   server.on("/api/report/", HTTP_GET, [](AsyncWebServerRequest* request) {
    
//     if(!makeDatabaseConnection()) {
//       Serial.println("FAILED CONNECTION IN UPDATE");
//       return;
//     };
    
//     MySQL_Cursor* cursorSelect = new MySQL_Cursor(&conn);
//     cursorSelect->execute(SELECT_LOCATOR_QUERY);
//     cursorSelect->get_columns();

//     StaticJsonDocument<2000> buffer;

//     JsonArray reportArray = buffer.to<JsonArray>();

//     row_values* row = NULL;
//     while((row = cursorSelect->get_next_row()) != NULL)
//       {
//         JsonArray nextReport = reportArray.createNestedArray();
//         nextReport.add(row->values[0]);
//         nextReport.add(strtoul(row->values[1], NULL, 10));
//         nextReport.add(strtoul(row->values[2], NULL, 10));
//       };

//     char jsonBuffer[2000];
//     serializeJson(buffer, jsonBuffer);
//     request->send_P(200, "application/json", jsonBuffer);

//     cursorSelect->close();
//     delete cursorSelect;
//     buffer.clear();
//   });

//   DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
//   server.begin();
// };


// void syncEspTime() {
//   struct tm datetimeObject;

//   if(!getLocalTime(&datetimeObject)) {
//     Serial.println("Failed to obtain time");
//     return;
//   };

//   struct timeval timeValueNow = {
//     .tv_sec = mktime(&datetimeObject)
//   };
//   settimeofday(&timeValueNow, NULL);
// }

// void updateDatabase() {
//   Serial.println("Per minute");

//   struct timeval timeValueNow;
//   gettimeofday(&timeValueNow, NULL);
//   struct tm* datetimeObject = gmtime(&timeValueNow.tv_sec);

//   char datetimeBuffer[20];
//   sprintf(
//     datetimeBuffer,
//     TIME_FORMAT,
//     datetimeObject->tm_year+1900,
//     datetimeObject->tm_mon+1,
//     datetimeObject->tm_mday,
//     datetimeObject->tm_hour,
//     datetimeObject->tm_min,
//     datetimeObject->tm_sec
//   );
//   Serial.print("StrTIME -> ");
//   Serial.println(datetimeBuffer);

//   uint8_t noisesPerInterval = random(0, 20),
//           disturbersPerInterval = random(0, 30);


//   if(!makeDatabaseConnection()) {
//     Serial.println("FAILED CONNECTION IN UPDATE");
//     return;
//   };
  // if(!conn.connect(conf.DATABASE_HOST, conf.DATABASE_PORT, conf.DATABASE_USER, conf.DATABASE_PASSWORD, conf.DATABASE_NAME))

  // Serial.println("Connection DONE");

//   MySQL_Cursor* cursorInsert = new MySQL_Cursor(&conn);
//   char queryBuffer[270];
//   sprintf(queryBuffer, INSERT_LOCATOR_QUERY_TEMPLATE, datetimeBuffer, noisesPerInterval, disturbersPerInterval);
//   cursorInsert->execute(queryBuffer);
//   cursorInsert->close();
//   delete cursorInsert;
  


//   const char dataTemplate[] = "{\"time\": \"%s\", \"noises\": %d, \"disturbers\": %d}";
//   sprintf(queryBuffer, dataTemplate, datetimeBuffer, noisesPerInterval, disturbersPerInterval);
//   ws.textAll(queryBuffer);
// };


void setup()
  {
    Serial.begin(115200);
    Serial.println();
    // EEPROM.begin(512);

    connectToWifi();

    HTTPClient http;
    http.begin("http://192.168.1.118/api/system/data/push/");
    http.addHeader(conf.API_KEY_NAME, conf.API_KEY_VALUE);
    http.addHeader("Content-Type", "application/json");
    int rc = http.POST("null");
    Serial.print("status -> ");
    Serial.print(rc);
  };

void loop()
  {

  };




// wsServerMonitor.loop();

// if(conn.connect(conf.DATABASE_HOST, conf.DATABASE_PORT, conf.DATABASE_USER, conf.DATABASE_PASSWORD, conf.DATABASE_NAME))
//   {
//     Serial.println("Done");
//     cursor.execute(query);
//     cursor.get_columns();

//     row_values* row = NULL;
//     do {
//       row = cursor.get_next_row();
//       if (row != NULL) {
//         Serial.println(row->values[0]);
//       }
//     } while (row != NULL);

//     cursor.close();
//     conn.close();
//   };


// wsServerMonitor.init();
// wsServerMonitor.run();


// struct tm datetimeObject;

//   if(!getLocalTime(&datetimeObject)) {
//     Serial.println("Failed to obtain time");
//     return;
//   };

//   time_t t = mktime(&timeinfo);

//   printf("Setting time: %s", asctime(&timeinfo));
//   struct timeval now = { .tv_sec = t };
//   settimeofday(&now, NULL);


// void setup()
//   {
//     Serial.begin(115200);
//     Serial.println();
//     // EEPROM.begin(512);

//     WiFi.config(conf.WIFI_IP, conf.WIFI_GATEWAY, conf.WIFI_SUBNET_MASK, conf.WIFI_PRIMARY_DNS, conf.WIFI_SECONDARY_DNS);
//     connectToWifi();

//     Serial.print("WiFI Connected -> ");
//     Serial.println(WiFi.localIP());

//     configTime(0, 0, "pool.ntp.org");
//     syncEspTime();

//     // char cronTab[] = "0 * * * * *";
//     char cronTab[] = "*/10 * * * * *";

//     Cron.create(cronTab, updateDatabase, false);

//     TaskHandle_t xCronJobHandler;
//     xTaskCreatePinnedToCore(
//       [](void* data) {
//         for(;;) {
//           Cron.delay();
//           vTaskDelay(10 / portTICK_PERIOD_MS);
//         };
//       },
//       "cron-job",
//       5000,
//       nullptr,
//       1,
//       &xCronJobHandler,
//       1
//     );

//     if(makeDatabaseConnection())
//       Serial.println("CONNECTED");

//     startWebServer();
//   };

// void loop()
//   {

//   };