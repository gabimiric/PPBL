#ifndef VENTILATION_FAN_H
#define VENTILATION_FAN_H

#include "Actuator.h"
#include "config.h"

// ============================================================================
// VENTILATION FAN ACTUATOR DRIVER
// ============================================================================

class VentilationFan : public Actuator
{
public:
    VentilationFan(uint8_t in1Pin = FAN_HBRIDGE_IN1_PIN,
                   uint8_t in2Pin = FAN_HBRIDGE_IN2_PIN,
                   int8_t enPin = FAN_HBRIDGE_EN_PIN);
    virtual ~VentilationFan() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "Ventilation Fan"; }
    uint8_t getModuleType() const override { return MODULE_FAN; }

    void on() override;
    void off() override;
    void toggle() override;

    bool isOn() const override { return _isOn; }
    unsigned long getRunTime() const override;
    uint8_t getPowerLevel() const override { return _isOn ? 255 : 0; }

    /**
     * Check if fan has been running too long (safety timeout).
     */
    bool isTimeoutActive() const;

    /**
     * Reset run time counter.
     */
    void resetRunTime();

private:
    uint8_t _in1Pin;
    uint8_t _in2Pin;
    int8_t _enPin;
    unsigned long _onStartTime = 0;

    /**
     * Perform safety checks before allowing fan to run.
     */
    bool performSafetyCheck();
};

#endif // VENTILATION_FAN_H
