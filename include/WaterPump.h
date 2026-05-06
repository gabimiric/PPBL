#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include "Actuator.h"
#include "config.h"

// ============================================================================
// WATER PUMP ACTUATOR DRIVER
// ============================================================================

class WaterPump : public Actuator
{
public:
    WaterPump(uint8_t relayPin = PUMP_RELAY_PIN);
    virtual ~WaterPump() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "Water Pump"; }
    uint8_t getModuleType() const override { return MODULE_PUMP; }

    void on() override;
    void off() override;
    void toggle() override;

    bool isOn() const override { return _isOn; }
    unsigned long getRunTime() const override;
    uint8_t getPowerLevel() const override { return _isOn ? 255 : 0; }

    /**
     * Check if pump has been running too long (safety timeout).
     */
    bool isTimeoutActive() const;

    /**
     * Reset run time counter.
     */
    void resetRunTime();

private:
    uint8_t _relayPin;
    unsigned long _onStartTime = 0;

    /**
     * Perform safety checks before allowing pump to run.
     */
    bool performSafetyCheck();
};

#endif // WATER_PUMP_H
