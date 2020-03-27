#include <Arduino.h>
#include "TemperatureSensor.h"
#include "PHSensor.h"
#include "TDSSensor.h"
#include "ORPSensor.h"
#include <WiFi.h>
#include "PubSubClient.h"
#include <driver/gpio.h>

// Wifi configuration
const char *ssid = "***";
const char *password = "***";

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

#ifdef BUSTED
#include "ArduinoOTA.h"

const int led = 2;

void blinkMe(const int count, const int pause = 1000)
{
  for (int i = 0; i < count; i++)
  {
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
    delay(pause);
    digitalWrite(led, HIGH);
    // only delay again if not the last one
    if (i + 1 < count)
    {
      delay(pause);
    }
  }
}

// ArduinoOTAClass ArduinoOTA;

void enableOTA()
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    pinMode(led, OUTPUT);
    blinkMe(5, 200);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    blinkMe(5, 200);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    if ((progress / total) % 20 == 0)
    {
      if (digitalRead(led) == HIGH)
      {
        digitalWrite(led, LOW);
      }
      else
      {
        digitalWrite(led, HIGH);
      }
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  stayAwake = true;
}
#endif

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

void setup()
{
  // maybe connect to ntp server for time occasionally? protocols/sntp example
  // check if rtc initialized, if not ntp client configure it, if frequency change of sompling is desired (less at night)
  // will only work if deep sleep hiberation is not used
  bootCount++;
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Boot number: " + String(bootCount));

  begin();
}

void loop()
{
  measureAndPublish();

  if (!stayAwake)
  {
    // for hibernation; RTC_DATA_ATTR may prevent rtc clock going off
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    // now go to sleep
    esp_sleep_enable_timer_wakeup(10 * 1e6);
    esp_deep_sleep_start();
  }
  else
  {
#ifdef BUSTED
    // should set a timer and turn off this mode after 5 minutes
    ArduinoOTA.handle();

    // remind user we are not deep sleeping
    if ((count++ % 10) == 0)
    {
      blinkMe(1, 5);
    }
#endif
    delay(1000);
  }
}
