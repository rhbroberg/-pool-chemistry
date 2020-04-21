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
#include "esp_task_wdt.h"
#include "esp_deep_sleep.h" // fix my deprecation

// MQTT configuration
//const char* server = "ubuntu-18-04-04";
const char *server = "192.168.1.210";
const char *temperatureTopic = "/sensors/proto/temperature";
const char *tdsTopic = "/sensors/proto/tds";
const char *phTopic = "/sensors/proto/ph";
const char *orpTopic = "/sensors/proto/orp";
const char *clientName = "com.brobasino.esp32";
const char *otaClientName = "myesp32";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
PushOTA ota;
RTC_DATA_ATTR int bootCount = 0;
volatile bool stayAwake = false; // isr modified

void wifiConnect()
{
  Serial.println();
  Serial.printf("Connecting WiFi to %s", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
  }
  Serial.printf("connected as %s\n", WiFi.localIP().toString().c_str());
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

void enableWatchdog()
{
  // these variables are defined in .platformio/packages/framework-arduinoespressif32/cores/esp32/main.cpp
  // this will enable the wrapper around loop() to feed the watchdog
  extern TaskHandle_t loopTaskHandle;
  extern bool loopTaskWDTEnabled;

  loopTaskWDTEnabled = true;
  // this should be a configuration parameter
  esp_err_t initWDTStatus = esp_task_wdt_init(10, true);
  esp_err_t wdtSelfStatus = esp_task_wdt_add(loopTaskHandle);

  if ((initWDTStatus != ESP_OK) || (wdtSelfStatus != ESP_OK))
  {
    Serial.print("watchdog configuration failed: ");
    Serial.println(initWDTStatus);
    Serial.println(wdtSelfStatus);
  }
  // enable idle-task watchdog watching/feeding as well to catch spins
  esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(0));
  esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(1));
}

void IRAM_ATTR otaTimeout()
{
  // strictly speaking the ISR should protect access to the variable; however it's just a boolean so I'm inclined to ignore a critical section
  //  portENTER_CRITICAL_ISR(&timerMux);
  stayAwake = false;
  //  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Boot number: " + String(bootCount));
  bootCount++;
  enableWatchdog();

  // some activities may need to be performed on a power-on reset which is ESP_RST_DEEPSLEEP;
  // ESP_RST_POWERON is other popular reason
  // maybe connect to ntp server for time occasionally? protocols/sntp example
  // check if rtc initialized, if not ntp client configure it, if frequency change of sompling is desired (less at night)
  // will only work if deep sleep hiberation is not used
  esp_reset_reason_t whyReset = esp_reset_reason();
  Serial.print("reset because ");
  Serial.println(whyReset);
  // if reset for any other reason probably add to list of mqtt topics to update for telemetry

  if (esp_sleep_get_wakeup_cause() == ESP_DEEP_SLEEP_WAKEUP_TOUCHPAD)
  {
    Serial.println("woken up due to touchpad");

    touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();
    Serial.print("pin was ");
    Serial.println(touchPin);

    //    if (touchPin == 5)
    {
      Serial.println("entering OTA (push) mode");
      ota.setNetworking(ssid, password, OTA_AUTH, otaClientName);
      stayAwake = true;
      ota.enable();

      // probably migrate this into the ota class
      // set a timer to go back to regular operation if no OTA performed within a timeout
      auto timer = timerBegin(0, 80, true);
      timerAttachInterrupt(timer, &otaTimeout, true);
      // should be a configuration parameter
      timerAlarmWrite(timer, 60 * 1000000, true); // 1 minute alarm
      timerAlarmEnable(timer);
    }
  }
  else
  {
    begin();

    measureAndPublish();
  }
}

void loop()
{
  if (stayAwake)
  {
    ota.handle();
    // should be configuration parameter
    delay(500);
    Serial.print(".");
  }
  else
  {
    Serial.println("goodnight sweet prince");
    // for hibernation; RTC_DATA_ATTR may prevent rtc clock going off; alternatively turning off rtc may corrupt RTC_DATA_ATTR
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    // should be configuration parameter
    unsigned touchThreshold = 40;
    // configure T5 (pin 12) for interrupt; callback is empty lambda
    touchAttachInterrupt(
        T0, []() {}, touchThreshold);
    touchAttachInterrupt(
        T7, []() {}, touchThreshold);
    // configure touch pins as eligible to wakeup
    esp_sleep_enable_touchpad_wakeup();

    // now go to sleep
    esp_sleep_enable_timer_wakeup(30 * 1e6);
    esp_deep_sleep_start();
  }
}
