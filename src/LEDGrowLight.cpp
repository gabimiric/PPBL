#include "LEDGrowLight.h"
#include "ModuleManager.h"
#include "PhotoresistorSensor.h"

LEDGrowLight::LEDGrowLight(uint8_t pwmPin)
    : _pwmPin(pwmPin) {}

bool LEDGrowLight::init()
{
    // Check if pin supports PWM
    // PWM pins on Arduino Uno: 3, 5, 6, 9, 10, 11
    bool isPWMPin = (_pwmPin == 3 || _pwmPin == 5 || _pwmPin == 6 ||
                     _pwmPin == 9 || _pwmPin == 10 || _pwmPin == 11);

    if (!isPWMPin)
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] FAILED - Pin ");
            Serial.print(_pwmPin);
            Serial.println(" does not support PWM");
        }
        return false;
    }

    pinMode(_pwmPin, OUTPUT);
    off(); // Start with light off

    // Note: previous versions tried to auto-detect the LED by measuring a
    // light-level delta on the photoresistor before/after turning on the LED.
    // That detection was unreliable because the Sensor abstraction caches
    // readings and the per-sensor update() throttle (LIGHT_READ_INTERVAL)
    // blocked the second sample from being a fresh reading. Trust the
    // ENABLE_LED_GROW_LIGHT compile-time flag and verify wiring visually.
    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[LEDGrowLight] Initialized on pin ");
        Serial.println(_pwmPin);
    }

    return true;
}

bool LEDGrowLight::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void LEDGrowLight::update()
{
    // LED on/off is owned by the system control logic in main.cpp,
    // which combines light-sensor readings with photoperiod constraints.
    // Calling updatePhotoperiod() here would fight that logic on every loop()
    // iteration. Photoperiod helpers (isInPhotoperiod / updatePhotoperiod)
    // remain available for callers that opt in to time-only control.
}

void LEDGrowLight::on()
{
    if (!_available)
        return;

    // Idempotent: a repeat call must not reset _onStartTime — keeps getRunTime()
    // monotonic and stops the debug log from spamming once per tick.
    if (_isOn)
        return;

    _isOn = true;
    _onStartTime = millis();
    if (_powerLevel == 0)
        _powerLevel = 255; // Default to full brightness
    analogWrite(_pwmPin, _powerLevel);

    if (DEBUG_ENABLED)
    {
        Serial.print("[LEDGrowLight] Turned ON (brightness: ");
        Serial.print((_powerLevel * 100) / 255);
        Serial.println("%)");
    }
}

void LEDGrowLight::off()
{
    if (_isOn)
    {
        unsigned long runTime = getRunTime();
        _isOn = false;
        _powerLevel = 0;
        _onStartTime = 0;
        analogWrite(_pwmPin, 0);

        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] Turned OFF (ran for ");
            Serial.print(runTime);
            Serial.println(" ms)");
        }
    }
}

void LEDGrowLight::toggle()
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

unsigned long LEDGrowLight::getRunTime() const
{
    if (!_isOn)
        return 0;
    return millis() - _onStartTime;
}

void LEDGrowLight::setBrightness(uint8_t level)
{
    if (_powerLevel == level)
    {
        return;
    }

    _powerLevel = level;

    if (_isOn)
    {
        analogWrite(_pwmPin, _powerLevel);

        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] Brightness set to ");
            Serial.print((_powerLevel * 100) / 255);
            Serial.println("%");
        }
    }
}

bool LEDGrowLight::isInPhotoperiod() const
{
    // This requires RTC or external time source for accurate time
    // For now, implement simple logic based on uptime
    // In production, use actual RTC (DS3231, etc.)

    uint32_t hours = getHoursSinceStartup();
    uint32_t hourOfDay = hours % 24;

    // Example: lights on from 6 AM to 10 PM (16 hours)
    return hourOfDay >= 6 && hourOfDay < 22;
}

uint32_t LEDGrowLight::getHoursSinceStartup() const
{
    return millis() / (1000 * 60 * 60);
}

void LEDGrowLight::updatePhotoperiod()
{
    // This will be improved with RTC integration
    // For now, it's called but does minimal work

    if (isInPhotoperiod())
    {
        if (!_isOn)
        {
            on();
        }
    }
    else
    {
        if (_isOn)
        {
            off();
        }
    }
}
