#include <Arduino.h>
#include <ArduinoJson.h>
#include <Settings.h>
#include <modules/Mqtt.h>


Mqtt::MqttConsumer mqttConsumer;
Mqtt::TopicPublisher dashboardPublisher(mqttConfig.SERVER_TOPIC_PUBLISH, mqttConsumer);


void callback(char* topic, JsonDocument& payload)
  {
    Serial.print("Topic -> ");
    Serial.println(topic);
    dashboardPublisher.yield("RETAIN");
  };

void setup()
  {
    Serial.begin(115200);
    Serial.println();

    mqttConsumer.configure(mqttConfig, callback);
    mqttConsumer.run();
  };

void loop()
  {

  };