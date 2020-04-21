#include "PushOTA.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "esp_task_wdt.h"

PushOTA::PushOTA()
    : _ssid(NULL), _password(NULL), _auth(NULL)
{
}

void PushOTA::setNetworking(const char *ssid, const char *password, const char *auth, const char *hostname)
{
    _ssid = ssid;
    _password = password;
    _auth = auth;
    _hostname = hostname;
}

// defaults to 3232
void PushOTA::setPort(const uint16_t port)
{
    ArduinoOTA.setPort(port);
}

const bool
PushOTA::enable()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _password);
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            Serial.println("failed to connect to wifi");
            return false;
        }
    }

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            // feed the watchdog, loop() not being exercised
            esp_task_wdt_reset();
        })
        .onError([](ota_error_t error) {
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

    if (_auth)
    {
        ArduinoOTA.setPassword(_auth);
    }
    if (_hostname)
    {
        // defaults is esp3232-[MAC]
        ArduinoOTA.setHostname(_hostname);
    }

    ArduinoOTA.begin();

    Serial.printf("Ready for OTA.  IP address: %s", WiFi.localIP().toString().c_str());

    return true;
}

void PushOTA::disable()
{
    ArduinoOTA.end();
    // disable wifi? if not successful probably have to
}

void PushOTA::handle()
{
    ArduinoOTA.handle();
}
