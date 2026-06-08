#include "SoilMoistureSensor.h"

SoilMoistureSensor::SoilMoistureSensor(uint8_t pin)
    : _pin(pin) {}

bool SoilMoistureSensor::init()
{
    // Verify pin is valid (analog pins A0-A5)
    if (!(_pin >= A0 && _pin <= A5))
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.print("[SoilMoistureSensor] FAILED - Invalid pin A");
            Serial.println(_pin - A0);
        }
        return false;
    }

    pinMode(_pin, INPUT);
    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[SoilMoistureSensor] Initialized on pin A");
        Serial.println(_pin - A0);
    }

    return true;
}

bool SoilMoistureSensor::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void SoilMoistureSensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime < SOIL_MOISTURE_READ_INTERVAL)
    {
        return; // Too soon to read again
    }

    // Average several samples to reduce ADC noise and unstable floating readings.
    uint32_t sum = 0;
    const uint8_t samples = 8;
    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(_pin);
        delayMicroseconds(250);
    }
    _rawValue = (uint16_t)(sum / samples);
    _value = getMoisturePercentage();
    _lastReadTime = now;
    _lastUpdateTime = now;

    if (DEBUG_ENABLED)
    {
        Serial.print("[SoilMoistureSensor] Pin A");
        Serial.print(_pin - A0);
        Serial.print(" - Raw: ");
        Serial.print(_rawValue);
        Serial.print(" | Moisture: ");
        Serial.print(_value);
        Serial.print("% | Status: ");

        if (isDry())
        {
            Serial.println("DRY");
        }
        else if (isWet())
        {
            Serial.println("WET");
        }
        else
        {
            Serial.println("MOIST");
        }

        // if (_rawValue <= 3)
        // {
        //     Serial.println("[SoilMoistureSensor] Warning: near-zero ADC. Check VCC/GND/AO wiring.");
        // }
        // else if (_rawValue >= 1020)
        // {
        //     Serial.println("[SoilMoistureSensor] Warning: near-max ADC. Sensor may be shorted or saturated.");
        // }
    }
}

bool SoilMoistureSensor::isFresh() const
{
    return (millis() - _lastReadTime) < SOIL_MOISTURE_READ_INTERVAL * 2;
}

float SoilMoistureSensor::getMoisturePercentage() const
{
    // Map raw value (dry_threshold to wet_threshold) to 0-100%.
    // Supports both polarities: wet can be higher or lower than dry.
    int dry = SOIL_MOISTURE_DRY_THRESHOLD;
    int wet = SOIL_MOISTURE_WET_THRESHOLD;

    if (dry == wet)
    {
        return 0.0f;
    }

    int minThreshold = dry < wet ? dry : wet;
    int maxThreshold = dry > wet ? dry : wet;

    if (_rawValue <= minThreshold)
    {
        return dry < wet ? 0.0f : 100.0f;
    }
    if (_rawValue >= maxThreshold)
    {
        return dry < wet ? 100.0f : 0.0f;
    }

    float normalized = ((float)(_rawValue - minThreshold) / (maxThreshold - minThreshold)) * 100.0f;
    return dry < wet ? normalized : (100.0f - normalized);
}

bool SoilMoistureSensor::isDry() const
{
    int dry = SOIL_MOISTURE_DRY_THRESHOLD;
    int wet = SOIL_MOISTURE_WET_THRESHOLD;
    return (dry < wet) ? (_rawValue <= dry) : (_rawValue >= dry);
}

bool SoilMoistureSensor::isWet() const
{
    int dry = SOIL_MOISTURE_DRY_THRESHOLD;
    int wet = SOIL_MOISTURE_WET_THRESHOLD;
    return (dry < wet) ? (_rawValue >= wet) : (_rawValue <= wet);
}
