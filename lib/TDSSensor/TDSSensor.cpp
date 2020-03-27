#include "Arduino.h"

long sampleTDS()
{
    const int analogInPin = 32;
    unsigned long int average = 0;

    // turn on sensor
    pinMode(26, OUTPUT);
    // disable internal pulldown resistor
    gpio_pulldown_dis(GPIO_NUM_26);

    digitalWrite(26, HIGH);
    delay(1500); // give sensor time to settle

    pinMode(2, ANALOG);
    for (int i = 0; i < 10; i++)
    {
        average += analogRead(analogInPin);
        delay(10);
    }

    float reading = (float)average / 10;
    Serial.print("tds = ");
    Serial.println(reading);

    digitalWrite(26, LOW);
    // enable internal pulldown resistor to keep peripheral off during deep sleep
    gpio_pulldown_en(GPIO_NUM_26);

    return (average);
}
