#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "Module.h"

// ============================================================================
// BASE CLASS FOR ACTUATORS
// ============================================================================

class Actuator : public Module
{
public:
    virtual ~Actuator() {}

    /**
     * Turn actuator ON.
     */
    virtual void on() = 0;

    /**
     * Turn actuator OFF.
     */
    virtual void off() = 0;

    /**
     * Toggle actuator state.
     */
    virtual void toggle() = 0;

    /**
     * Check if actuator is currently ON.
     */
    virtual bool isOn() const = 0;

    /**
     * Get time actuator has been running (ms).
     * Useful for safety timeouts.
     */
    virtual unsigned long getRunTime() const = 0;

    /**
     * Get actuator power level (0-255 for PWM, or 0/255 for digital).
     */
    virtual uint8_t getPowerLevel() const = 0;

protected:
    bool _isOn = false;
    unsigned long _onTime = 0;
    uint8_t _powerLevel = 0;
};

#endif // ACTUATOR_H
