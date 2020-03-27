#include "Arduino.h"

long samplePH()
{
    const int analogInPin = 33;
    const int powerPin = 25;
    unsigned long int average = 0;

    pinMode(powerPin, OUTPUT);
    // disable internal pulldown resistor
    gpio_pulldown_dis(GPIO_NUM_25);
    pinMode(analogInPin, ANALOG);
    // turn on sensor
    digitalWrite(powerPin, HIGH);
    delay(1000); // give sensor time to settle

    for (int i = 0; i < 10; i++)
    {
        average += analogRead(analogInPin);
        delay(10);
    }

    float reading = (float)average / 10;
    Serial.print("ph = ");
    Serial.println(reading);

    digitalWrite(powerPin, LOW);
    // enable internal pulldown resistor to keep peripheral off during deep sleep
    gpio_pulldown_en(GPIO_NUM_25);

    pinMode(analogInPin, OUTPUT_OPEN_DRAIN);

    return (average);
}
