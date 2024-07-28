/*
     Example of connection using Static IP
     by Evandro Luis Copercini
     Public domain - 2017
*/

#include <WiFi.h>
#include "driver/rtc_io.h"
#include <MQTT.h>

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define USE_EXT0_WAKEUP          1               // 1 = EXT0 wakeup, 0 = EXT1 wakeup
#define WAKEUP_GPIO              GPIO_NUM_33     // Only RTC IO are allowed - ESP32 Pin example

const char *ssid = "ESSID";
const char *password = "PASSWORD";
const char *mqtt_host = "MQTTHOST";

// fixed IP for faster connection
IPAddress local_IP(10, 0, 0, 55);
IPAddress gateway(10, 0, 0, 10);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 10, 10, 10);    //optional

const char *discovery_topic = "homeassistant/device_automation/doorbell_1/config";
const char *discovery_payload = "{\"automation_type\":\"trigger\",\"type\":\"action\",\"subtype\":\"ring\",\"payload\":\"ring\",\"topic\":\"custom/doorbell_1\",\"device\":{\"identifiers\":[\"custom_doorbell_1\"],\"name\":\"doorbell_1\",\"sw_version\":\"0.2\",\"model\":\"Grothe ESP Bell\",\"manufacturer\":\"chaosbiber\"}}";
const char *bell_topic = "custom/doorbell_1"
const char *bell_payload = "ring"

bool wificonnect() {
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  delay(10);

  Serial.print("Connecting to ssid ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  wl_status_t state;
  unsigned long start = millis();
  state = WiFi.status();
  while (state != WL_CONNECTED) {
    if (state == WL_CONNECT_FAILED || millis() - start > 3000) {
      Serial.println("Disconnecting WiFi");
      WiFi.disconnect(true);
      delay(100);
      return false;
    }
    delay(100);
    Serial.print(".");
    state = WiFi.status();
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  bool connected = wificonnect();
  if (!connected) {
    connected = wificonnect(); //2nd try especially if WiFi.disconnect wasn't called in the last loop
  }

  if (connected) {
    WiFiClient net;
    MQTTClient client;
    client.begin(mqtt_host, net);

    while (!client.connect("arduino", "client", "M47Hsy7Y")) {
      Serial.print(":");
      delay(100);
    }

    delay(10);
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
      client.publish(bell_topic, bell_payload, false, 1);
      client.loop();
      delay(10);
      client.disconnect();

      // Blink codes for feedback but also waiting helped to ensure the mqtt message got through.
      // Maybe not needed with qos = 1 param
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
    } else { // Wakeup most likely from connecting to power - send Home Assistant setup topic
      client.publish(discovery_topic, discovery_payload, false, 1);
      client.loop();
      delay(10);
      // send debug message with wakeup reason to bell topic, not triggering home assistant
      char buffer[20];
      sprintf(buffer, "{\"wakeup_reason\":%d}", wakeup_reason);
      client.publish(bell_topic, buffer, false, 1);
      client.loop();
      delay(10);

      client.disconnect();

      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
    }
    WiFi.disconnect(true);
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println();
  Serial.println(millis());

  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);  //1 = High, 0 = Low
  // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
  // EXT0 resides in the same power domain (RTC_PERIPH) as the RTC IO pullup/downs.
  // No need to keep that power domain explicitly, unlike EXT1.
  /* GPIO 34 + 35 require external pullup/down, 32, 33 and other RTC-based inputs can use internal */
  rtc_gpio_pullup_dis(WAKEUP_GPIO);
  rtc_gpio_pulldown_dis(WAKEUP_GPIO);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

void loop() {
  // unreachable
  // blink to indicate error
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
