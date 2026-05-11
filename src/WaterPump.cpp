#include "WaterPump.h"

WaterPump::WaterPump(uint8_t relayPin)
    : _relayPin(relayPin) {}

bool WaterPump::init()
{
    // Verify pin is valid before initializing (Uno digital range D0..D13).
    if (_relayPin > 13)
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.print("[WaterPump] FAILED - Invalid pin ");
            Serial.println(_relayPin);
        }
        return false;
    }

    pinMode(_relayPin, OUTPUT);
    off(); // Start with pump off

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[WaterPump] Initialized on pin ");
        Serial.println(_relayPin);
    }

    return true;
}

bool WaterPump::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void WaterPump::update()
{
    // Check for safety timeout
    if (_isOn && isTimeoutActive())
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[WaterPump] TIMEOUT - Pump forced OFF!");
        }
        off();
    }
}

void WaterPump::on()
{
    if (!_available)
        return;

    // Idempotent: a repeat call must not reset _onStartTime, or the safety
    // timeout would never expire when control logic calls on() every loop tick.
    if (_isOn)
        return;

    if (!performSafetyCheck())
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[WaterPump] Safety check failed - pump NOT activated");
        }
        return;
    }

    _isOn = true;
    _onStartTime = millis();
    _powerLevel = 255;
    digitalWrite(_relayPin, HIGH);

    if (DEBUG_ENABLED)
    {
        Serial.println("[WaterPump] Turned ON");
    }
}

void WaterPump::off()
{
    if (_isOn)
    {
        unsigned long runTime = getRunTime();
        _isOn = false;
        _powerLevel = 0;
        _onStartTime = 0;
        digitalWrite(_relayPin, LOW);

        if (DEBUG_ENABLED)
        {
            Serial.print("[WaterPump] Turned OFF (ran for ");
            Serial.print(runTime);
            Serial.println(" ms)");
        }
    }
}

void WaterPump::toggle()
{
    if (_isOn)
    {
        off();
    }
    else
    {
        on();
    }
}

unsigned long WaterPump::getRunTime() const
{
    if (!_isOn)
        return 0;
    return millis() - _onStartTime;
}

bool WaterPump::isTimeoutActive() const
{
    if (!_isOn)
        return false;
    return getRunTime() > PUMP_MAX_ON_TIME;
}

void WaterPump::resetRunTime()
{
    _onStartTime = millis();
}

bool WaterPump::performSafetyCheck()
{
    // Add any safety checks here
    // For example: check soil moisture before pumping
    // This prevents overwatering
    return true;
}
