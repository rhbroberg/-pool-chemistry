#include <OneWire.h>
#include <DallasTemperature.h>
#include "Arduino.h"

// Data wire is connected to GPIOXX
#define ONE_WIRE_BUS 16
// Setup a oneWire instance to communicate with a OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
DeviceAddress firstTempSensor;

const double
sampleTemperature()
{
    digitalWrite(27, HIGH);
    Serial.print("Requesting temperatures...");
    if (!sensors.requestTemperaturesByAddress(firstTempSensor))
    {
        Serial.println("failed");
    }
    else
    {
        Serial.println("DONE");
    }

    double temperature = sensors.getTempC(firstTempSensor);
    Serial.print("Sensor temp: ");
    Serial.println(temperature);

    digitalWrite(27, LOW);
    return temperature;
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16)
        {
            Serial.print("0");
        }
        Serial.print(deviceAddress[i], HEX);
    }
}

void begin()
{
    // temperature power
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);
    sensors.begin();

    // search for devices on the bus and assign based on an index.
    if (!sensors.getAddress(firstTempSensor, 0))
    {
        Serial.println("Unable to find address for Device 0");
    }

    sensors.setResolution(firstTempSensor, 9);
    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(firstTempSensor);
    Serial.println();
    digitalWrite(27, LOW);
}
