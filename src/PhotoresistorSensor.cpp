#include "PhotoresistorSensor.h"

#include <math.h>

namespace
{
constexpr float ADC_MAX_VALUE = 1023.0f;
constexpr float LDR_FIXED_RESISTOR_OHMS = 10000.0f;
constexpr float LUX_SCALE = 50000000.0f;
constexpr float LUX_EXPONENT = 1.4f;

float adcToLux(uint16_t raw)
{
    if (raw < 1)
    {
        raw = 1;
    }
    else if (raw >= 1023)
    {
        raw = 1022;
    }

    float resistance = LDR_FIXED_RESISTOR_OHMS * (float)raw / (ADC_MAX_VALUE - (float)raw);
    float lux = LUX_SCALE / powf(resistance, LUX_EXPONENT);

    if (lux < 0.0f)
    {
        lux = 0.0f;
    }

    return lux;
}
}

PhotoresistorSensor::PhotoresistorSensor(uint8_t pin)
    : _pin(pin) {}

bool PhotoresistorSensor::init()
{
    pinMode(_pin, INPUT);

    // Take an initial reading and store it in _value so that any module that
    // queries getValue() before the first update() gets a real lux estimate.
    uint16_t reading = analogRead(_pin);
    _value = adcToLux(reading);
    _lastReadTime = millis();

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[PhotoresistorSensor] Initialized on pin A");
        Serial.print(_pin - A0);
        Serial.print(" (initial raw: ");
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
    _value = adcToLux(reading);
    _lastReadTime = now;
    _lastUpdateTime = now;

    if (DEBUG_ENABLED)
    {
        Serial.print("[PhotoresistorSensor] Light level: ");
        Serial.print(_value);
        Serial.println(" lux");
    }
}

bool PhotoresistorSensor::isFresh() const
{
    return (millis() - _lastReadTime) < LIGHT_READ_INTERVAL * 2;
}

bool PhotoresistorSensor::needsSupplementalLight() const
{
    return _value < LIGHT_LEVEL_THRESHOLD;
}
