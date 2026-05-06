#include "VentilationFan.h"

VentilationFan::VentilationFan(uint8_t in1Pin, uint8_t in2Pin, int8_t enPin)
    : _in1Pin(in1Pin), _in2Pin(in2Pin), _enPin(enPin) {}

bool VentilationFan::init()
{
    // Verify H-bridge input pins are valid before initializing.
    if (_in1Pin > 13 || _in2Pin > 13 || _in1Pin == _in2Pin)
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.println("[VentilationFan] FAILED - Invalid H-bridge input pins");
        }
        return false;
    }

    if (_enPin >= 0 && _enPin > 13)
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.println("[VentilationFan] FAILED - Invalid H-bridge enable pin");
        }
        return false;
    }

    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    if (_enPin >= 0)
    {
        pinMode(_enPin, OUTPUT);
    }
    off(); // Start with fan off

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[VentilationFan] Initialized on H-bridge pins IN1=");
        Serial.print(_in1Pin);
        Serial.print(", IN2=");
        Serial.print(_in2Pin);
        Serial.print(", EN=");
        Serial.println(_enPin);
    }

    return true;
}

bool VentilationFan::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void VentilationFan::update()
{
    // Force-maintain EN and direction pins if motor should be on
    if (_isOn)
    {
        if (_enPin >= 0)
        {
            analogWrite(_enPin, 255); // PWM at full strength
        }
        digitalWrite(_in1Pin, HIGH);
        digitalWrite(_in2Pin, LOW);
    }
    
    // Check for safety timeout
    if (_isOn && isTimeoutActive())
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[VentilationFan] TIMEOUT - Fan forced OFF!");
        }
        off();
    }
}

void VentilationFan::on()
{
    if (DEBUG_ENABLED)
    {
        Serial.println("[VentilationFan] on() called!");
    }

    if (!_available)
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[VentilationFan] on() called but module not available!");
        }
        return;
    }

    if (!performSafetyCheck())
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[VentilationFan] Safety check failed - fan NOT activated");
        }
        return;
    }

    _isOn = true;
    _onStartTime = millis();
    _powerLevel = 255;

    // Set EN pin using PWM to enable H-bridge
    if (_enPin >= 0)
    {
        analogWrite(_enPin, 255); // PWM at full strength
    }

    // Small delay to ensure EN is stable
    delayMicroseconds(100);

    // Then set direction pins
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, LOW);

    if (DEBUG_ENABLED)
    {
        Serial.print("[VentilationFan] ON - EN=PWM(255), IN1(");
        Serial.print(_in1Pin);
        Serial.print(")=HIGH, IN2(");
        Serial.print(_in2Pin);
        Serial.println(")=LOW");
    }
}

void VentilationFan::off()
{
    if (_isOn)
    {
        unsigned long runTime = getRunTime();
        _isOn = false;
        _powerLevel = 0;
        _onStartTime = 0;

        // Set both IN pins LOW first
        digitalWrite(_in1Pin, LOW);
        digitalWrite(_in2Pin, LOW);

        // Then disable EN using PWM
        if (_enPin >= 0)
        {
            analogWrite(_enPin, 0); // PWM to 0 to disable
        }

        if (DEBUG_ENABLED)
        {
            Serial.print("[VentilationFan] OFF (ran for ");
            Serial.print(runTime);
            Serial.println(" ms)");
        }
    }
}

void VentilationFan::toggle()
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

unsigned long VentilationFan::getRunTime() const
{
    if (!_isOn)
        return 0;
    return millis() - _onStartTime;
}

bool VentilationFan::isTimeoutActive() const
{
    if (!_isOn)
        return false;
    return getRunTime() > FAN_MAX_ON_TIME;
}

void VentilationFan::resetRunTime()
{
    _onStartTime = millis();
}

bool VentilationFan::performSafetyCheck()
{
    // Add any safety checks here
    return true;
}
