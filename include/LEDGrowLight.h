#ifndef LED_GROW_LIGHT_H
#define LED_GROW_LIGHT_H

#include "Actuator.h"
#include "config.h"

// ============================================================================
// LED GROW LIGHT ACTUATOR DRIVER
// ============================================================================

class LEDGrowLight : public Actuator
{
public:
    LEDGrowLight(uint8_t pwmPin = LED_LIGHT_PIN);
    virtual ~LEDGrowLight() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "LED Grow Light"; }
    uint8_t getModuleType() const override { return MODULE_LED_LIGHT; }

    void on() override;
    void off() override;
    void toggle() override;

    bool isOn() const override { return _isOn; }
    unsigned long getRunTime() const override;
    uint8_t getPowerLevel() const override { return _powerLevel; }

    /**
     * Set brightness level (0-255 PWM).
     */
    void setBrightness(uint8_t level);

    /**
     * Get current brightness level.
     */
    uint8_t getBrightness() const { return _powerLevel; }

    /**
     * Check if current time falls within photoperiod.
     */
    bool isInPhotoperiod() const;

    /**
     * Get hours since startup (requires RTC for accurate time).
     */
    uint32_t getHoursSinceStartup() const;

private:
    uint8_t _pwmPin;
    unsigned long _onStartTime = 0;

    /**
     * Control light based on photoperiod (if RTC available).
     */
    void updatePhotoperiod();
};

#endif // LED_GROW_LIGHT_H
