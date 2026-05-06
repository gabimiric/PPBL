#include "BH1750Sensor.h"
#include <Wire.h>

BH1750Sensor::BH1750Sensor(uint8_t i2cAddress)
    : _i2cAddress(i2cAddress) {}

bool BH1750Sensor::init()
{
    // Initialize I2C
    Wire.begin();

    // Try to initialize BH1750
    if (!sendCommand(0x01))
    { // Power on command
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.println("[BH1750] Failed to initialize - sensor not responding");
        }
        return false;
    }

    _available = true;

    // Set measurement mode to continuously high resolution
    sendCommand(0x10); // Continuous H-resolution mode

    if (DEBUG_ENABLED)
    {
        Serial.println("[BH1750] Initialized successfully");
    }

    return true;
}

bool BH1750Sensor::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void BH1750Sensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime < LIGHT_READ_INTERVAL)
    {
        return; // Too soon to read again
    }

    if (readLux())
    {
        _lastReadTime = now;

        if (DEBUG_ENABLED)
        {
            Serial.print("[BH1750] Light Level: ");
            Serial.print(_value);
            Serial.println(" lux");
        }
    }
    else if (DEBUG_ENABLED)
    {
        Serial.println("[BH1750] Read failed!");
    }

    _lastUpdateTime = now;
}

bool BH1750Sensor::isFresh() const
{
    return (millis() - _lastReadTime) < LIGHT_READ_INTERVAL * 2;
}

bool BH1750Sensor::needsSupplementalLight() const
{
    return _value < LIGHT_LEVEL_THRESHOLD;
}

bool BH1750Sensor::sendCommand(uint8_t cmd)
{
    Wire.beginTransmission(_i2cAddress);
    Wire.write(cmd);
    return Wire.endTransmission() == 0;
}

bool BH1750Sensor::readLux()
{
    Wire.requestFrom(_i2cAddress, (uint8_t)2);

    if (Wire.available() < 2)
    {
        return false;
    }

    uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();

    // Convert raw value to lux
    // BH1750 resolution: 1.2 lux/count in high resolution mode
    _value = raw / 1.2f;

    return true;
}
