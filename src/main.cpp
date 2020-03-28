#include <Arduino.h>
#include "TemperatureSensor.h"
#include "PHSensor.h"
#include "TDSSensor.h"
#include "ORPSensor.h"
#include <WiFi.h>
#include "PubSubClient.h"
#include <driver/gpio.h>
#include "PushOTA.h"
#include "passwords.h"

// MQTT configuration
//const char* server = "ubuntu-18-04-04";
const char *server = "192.168.1.210";
const char *temperatureTopic = "/sensors/proto/temperature";
const char *tdsTopic = "/sensors/proto/tds";
const char *phTopic = "/sensors/proto/ph";
const char *orpTopic = "/sensors/proto/orp";
const char *clientName = "com.brobasino.esp32";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

bool stayAwake = false;

void wifiConnect()
{
  Serial.println();
  Serial.print("Connecting WiFi to ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
  }
  Serial.print("connected.  IP address:");
  Serial.println(WiFi.localIP());
}

void mqttReConnect()
{
  while (!client.connected())
  {
    Serial.print("connecting to mqtt...");
    if (client.connect(clientName))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttEmit(String topic, String value)
{
  client.publish((const char *)topic.c_str(), (const char *)value.c_str(), false);
}

void measureAndPublish()
{
  const long ph = samplePH();
  const double temperature = sampleTemperature();
  const long tds = sampleTDS();
  const long orp = sampleORP();

  wifiConnect();
  client.setServer(server, 1883);

  if (!client.connected())
  {
    mqttReConnect();
  }
  mqttEmit(temperatureTopic, (String)temperature);
  mqttEmit(phTopic, (String)ph);
  mqttEmit(tdsTopic, (String)tds);
  mqttEmit(orpTopic, (String)orp);

  delay(1000); // icka.  needed?
  WiFi.mode(WIFI_OFF);
}

unsigned count = 0;
RTC_DATA_ATTR int bootCount = 0;
PushOTA ota;

void setup()
{
  bootCount++;
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial.println("Boot number: " + String(bootCount));

  // some activities may need to be performed on a power-on reset which is ESP_RST_DEEPSLEEP;
  // ESP_RST_POWERON is other popular reason
  // maybe connect to ntp server for time occasionally? protocols/sntp example
  // check if rtc initialized, if not ntp client configure it, if frequency change of sompling is desired (less at night)
  // will only work if deep sleep hiberation is not used
  esp_reset_reason_t whyReset = esp_reset_reason();
  Serial.print("reset because ");
  Serial.println(whyReset);

  begin();

#ifdef NOT_YET
  ota.setNetworking(ssid, password, OTA_AUTH);
  stayAwake = false;
  ota.enable();
#endif
}

void loop()
{
  measureAndPublish();

  if (!stayAwake)
  {
    // for hibernation; RTC_DATA_ATTR may prevent rtc clock going off; alternatively turning off rtc may corrupt RTC_DATA_ATTR
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    // now go to sleep
    esp_sleep_enable_timer_wakeup(10 * 1e6);
    esp_deep_sleep_start();
  }
  else
  {
    ota.handle();
    delay(1000);
  }
}
