#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#include "Actuator.h"
#include "config.h"

// ============================================================================
// 28BYJ-48 STEPPER MOTOR WITH ULN2003 DRIVER
// ============================================================================

class StepperMotor : public Actuator
{
public:
    StepperMotor(uint8_t in1Pin = STEPPER_IN1_PIN,
                 uint8_t in2Pin = STEPPER_IN2_PIN,
                 uint8_t in3Pin = STEPPER_IN3_PIN,
                 uint8_t in4Pin = STEPPER_IN4_PIN);
    virtual ~StepperMotor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "28BYJ-48 Stepper Motor"; }
    uint8_t getModuleType() const override { return MODULE_STEPPER_MOTOR; }

    void on() override;
    void off() override;
    void toggle() override;

    bool isOn() const override { return _isRunning; }
    unsigned long getRunTime() const override;
    uint8_t getPowerLevel() const override { return _isRunning ? 255 : 0; }

    /**
     * Rotate motor by specified number of steps.
     * Positive = clockwise, negative = counter-clockwise.
     */
    void rotate(int16_t steps);

    /**
     * Set rotation speed (RPM, default 15).
     */
    void setSpeed(uint8_t rpm);

    /**
     * Get current position in steps.
     */
    int32_t getCurrentPosition() const { return _currentPosition; }

    /**
     * Reset position counter to zero.
     */
    void resetPosition() { _currentPosition = 0; }

    /**
     * De-energize coils (reduces heat and power consumption when idle).
     */
    void deenergize();

private:
    uint8_t _in1Pin, _in2Pin, _in3Pin, _in4Pin;
    int32_t _currentPosition = 0;
    int16_t _stepsToMove = 0;
    bool _isRunning = false;
    unsigned long _motorStartTime = 0;
    uint8_t _currentStep = 0;
    unsigned long _lastStepTime = 0;
    uint8_t _rpm = 4; // Default speed (4 RPM - slow for half-step torque)

    /**
     * Stepper step patterns for half-step sequence (8 coils for more torque).
     */
    static const uint8_t STEP_PATTERN[8];

    /**
     * Execute one step.
     */
    void step();

    /**
     * Get delay between steps based on RPM.
     */
    unsigned long getStepDelay() const;
};

#endif // STEPPER_MOTOR_H
