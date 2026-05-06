#include "PhotoresistorSensor.h"

PhotoresistorSensor::PhotoresistorSensor(uint8_t pin)
    : _pin(pin) {}

bool PhotoresistorSensor::init()
{
    pinMode(_pin, INPUT);
    
    // Try to read the sensor
    uint16_t reading = analogRead(_pin);
    
    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[PhotoresistorSensor] Initialized on pin A");
        Serial.print(_pin - A0);
        Serial.print(" (initial reading: ");
        Serial.print(reading);
        Serial.println(")");
    }

    return true;
}

bool PhotoresistorSensor::isAvailable()
{
    return _available;
}

void PhotoresistorSensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime < LIGHT_READ_INTERVAL)
    {
        return; // Too soon to read again
    }

    uint16_t reading = analogRead(_pin);
    _value = (float)reading;
    _lastReadTime = now;
    _lastUpdateTime = now;

    if (DEBUG_ENABLED)
    {
        Serial.print("[PhotoresistorSensor] Light level: ");
        Serial.println(reading);
    }
}

bool PhotoresistorSensor::isFresh() const
{
    return (millis() - _lastReadTime) < LIGHT_READ_INTERVAL * 2;
}

bool PhotoresistorSensor::needsSupplementalLight() const
{
    // Higher ADC value = darker (inverted logic)
    return _value > LIGHT_LEVEL_THRESHOLD;
}
