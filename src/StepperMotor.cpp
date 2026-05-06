#include "StepperMotor.h"

// Half-step sequence for 28BYJ-48 (8-step pattern for more torque)
const uint8_t StepperMotor::STEP_PATTERN[8] = {
    0b1000, // IN1=1, IN2=0, IN3=0, IN4=0
    0b1100, // IN1=1, IN2=1, IN3=0, IN4=0
    0b0100, // IN1=0, IN2=1, IN3=0, IN4=0
    0b0110, // IN1=0, IN2=1, IN3=1, IN4=0
    0b0010, // IN1=0, IN2=0, IN3=1, IN4=0
    0b0011, // IN1=0, IN2=0, IN3=1, IN4=1
    0b0001, // IN1=0, IN2=0, IN3=0, IN4=1
    0b1001  // IN1=1, IN2=0, IN3=0, IN4=1
};

StepperMotor::StepperMotor(uint8_t in1Pin, uint8_t in2Pin, uint8_t in3Pin, uint8_t in4Pin)
    : _in1Pin(in1Pin), _in2Pin(in2Pin), _in3Pin(in3Pin), _in4Pin(in4Pin) {}

bool StepperMotor::init()
{
    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    pinMode(_in3Pin, OUTPUT);
    pinMode(_in4Pin, OUTPUT);

    // De-energize coils
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[StepperMotor] Initialized on pins ");
        Serial.print(_in1Pin);
        Serial.print(", ");
        Serial.print(_in2Pin);
        Serial.print(", ");
        Serial.print(_in3Pin);
        Serial.print(", ");
        Serial.println(_in4Pin);
    }

    return true;
}

bool StepperMotor::isAvailable()
{
    return _available;
}

void StepperMotor::update()
{
    // If no steps to move, de-energize motor to prevent heating
    if (_stepsToMove == 0)
    {
        if (_isRunning)
        {
            _isRunning = false;
            deenergize();  // De-energize coils when rotation complete
            if (DEBUG_ENABLED)
            {
                Serial.print("[StepperMotor] Rotation complete. Position: ");
                Serial.println(_currentPosition);
            }
        }
        return;
    }

    unsigned long now = millis();
    unsigned long stepDelay = getStepDelay();

    // Execute next step if enough time has passed
    if (now - _lastStepTime >= stepDelay)
    {
        step();
        _lastStepTime = now;
        _stepsToMove--;
    }
}

void StepperMotor::on()
{
    // For stepper, "on" means start a full rotation (4096 steps for 28BYJ-48)
    rotate(4096);
}

void StepperMotor::off()
{
    // Stop all motion and de-energize
    _stepsToMove = 0;
    _isRunning = false;
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);
}

void StepperMotor::toggle()
{
    if (_isRunning)
    {
        off();
    }
    else
    {
        on();
    }
}

void StepperMotor::rotate(int16_t steps)
{
    _stepsToMove = steps;
    _isRunning = (steps != 0);
    _lastStepTime = millis();

    if (DEBUG_ENABLED)
    {
        Serial.print("[StepperMotor] Starting rotation: ");
        Serial.print(steps);
        Serial.println(" steps");
    }
}

void StepperMotor::setSpeed(uint8_t rpm)
{
    // Limit to reasonable speeds to avoid stalling or excessive heat
    if (rpm < 1)
        rpm = 1;
    if (rpm > 30)
        rpm = 30;
    
    _rpm = rpm;
    if (DEBUG_ENABLED)
    {
        Serial.print("[StepperMotor] Speed set to ");
        Serial.print(rpm);
        Serial.println(" RPM");
    }
}

unsigned long StepperMotor::getRunTime() const
{
    if (!_isRunning)
        return 0;
    return millis() - _motorStartTime;
}

unsigned long StepperMotor::getStepDelay() const
{
    // 28BYJ-48 has 4096 full steps per revolution = 8192 half-steps per revolution
    // Using half-step sequence for better torque
    // Delay in ms = (60000 ms/min) / (half-steps/rev * RPM)
    // = 60000 / (8192 * RPM) for half-steps
    if (_rpm == 0)
        return 1000;
    return (60000 / (8192 * _rpm)) + 1; // +1 to maintain same speed as before
}

void StepperMotor::step()
{
    // Determine direction based on sign of steps remaining
    if (_stepsToMove > 0)
    {
        _currentStep = (_currentStep + 1) % 8; // Forward (8 half-steps)
        _currentPosition++;
    }
    else if (_stepsToMove < 0)
    {
        _currentStep = (_currentStep + 7) % 8; // Backward (equivalent to -1 mod 8)
        _currentPosition--;
    }

    // Apply step pattern to coils
    uint8_t pattern = STEP_PATTERN[_currentStep];
    digitalWrite(_in1Pin, (pattern >> 3) & 1);
    digitalWrite(_in2Pin, (pattern >> 2) & 1);
    digitalWrite(_in3Pin, (pattern >> 1) & 1);
    digitalWrite(_in4Pin, pattern & 1);

    if (DEBUG_ENABLED)
    {
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 500)  // Debug every 500ms
        {
            Serial.print("[StepperMotor] HalfStep ");
            Serial.print(_currentStep);
            Serial.print(" | Pins: ");
            Serial.print((pattern >> 3) & 1);
            Serial.print((pattern >> 2) & 1);
            Serial.print((pattern >> 1) & 1);
            Serial.print(pattern & 1);
            Serial.print(" | Pos: ");
            Serial.print(_currentPosition);
            Serial.print(" | Remaining: ");
            Serial.println(_stepsToMove);
            lastDebug = millis();
        }
    }
}

void StepperMotor::deenergize()
{
    // De-energize all coils to reduce heat and power consumption
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);
    
    if (DEBUG_ENABLED)
    {
        Serial.println("[StepperMotor] Coils de-energized");
    }
}
