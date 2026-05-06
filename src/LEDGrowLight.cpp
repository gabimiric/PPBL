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
    delay(100); // Wait for sensor to stabilize

    // Attempt hardware detection by measuring light level change
    ModuleManager &manager = ModuleManager::getInstance();
    Sensor *lightSensor = manager.getSensor(MODULE_LIGHT_SENSOR);
    
    if (lightSensor && lightSensor->isAvailable())
    {
        // Take baseline reading with LED off
        uint16_t baselineLight = (uint16_t)lightSensor->getValue();
        
        // Turn LED on to test brightness
        analogWrite(_pwmPin, 200);
        delay(150);  // Wait for light to stabilize and sensor to read
        
        // Take reading with LED on
        uint16_t ledOnLight = (uint16_t)lightSensor->getValue();
        
        // Turn LED off
        analogWrite(_pwmPin, 0);
        
        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] Detection test - Baseline: ");
            Serial.print(baselineLight);
            Serial.print(" | LED On: ");
            Serial.println(ledOnLight);
        }
        
        // If light level changed significantly (at least 20 points), LED is present
        // Remember: higher raw value = darker, so LED ON should make value LOWER
        if ((baselineLight - ledOnLight) >= 20)
        {
            _available = true;
            if (DEBUG_ENABLED)
            {
                Serial.println("[LEDGrowLight] LED DETECTED and initialized on pin " + String(_pwmPin));
            }
            return true;
        }
        else
        {
            _available = false;
            if (DEBUG_ENABLED)
            {
                Serial.print("[LEDGrowLight] FAILED - LED not detected on pin ");
                Serial.println(_pwmPin);
            }
            return false;
        }
    }
    else
    {
        // Light sensor not available, assume LED is there if pin is valid
        _available = true;
        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] Initialized on pin ");
            Serial.println(_pwmPin);
            Serial.println("[LEDGrowLight] (No light sensor for detection, assuming LED is present)");
        }
        return true;
    }
}

bool LEDGrowLight::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void LEDGrowLight::update()
{
    updatePhotoperiod();
}

void LEDGrowLight::on()
{
    if (!_available)
        return;

    // Only print if state is changing from OFF to ON
    if (!_isOn)
    {
        if (DEBUG_ENABLED)
        {
            Serial.print("[LEDGrowLight] Turned ON (brightness: ");
            Serial.print((_powerLevel * 100) / 255);
            Serial.println("%)");
        }
    }

    _isOn = true;
    _onStartTime = millis();
    if (_powerLevel == 0)
        _powerLevel = 255; // Default to full brightness
    analogWrite(_pwmPin, _powerLevel);
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
