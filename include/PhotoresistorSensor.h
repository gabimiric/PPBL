#ifndef PHOTORESISTOR_SENSOR_H
#define PHOTORESISTOR_SENSOR_H

#include "Sensor.h"
#include "config.h"

// ============================================================================
// PHOTORESISTOR (LDR) LIGHT SENSOR DRIVER
// ============================================================================

class PhotoresistorSensor : public Sensor
{
public:
    PhotoresistorSensor(uint8_t pin = LIGHT_SENSOR_PIN);
    virtual ~PhotoresistorSensor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "Photoresistor Light Sensor"; }
    uint8_t getModuleType() const override { return MODULE_LIGHT_SENSOR; }

    /**
     * getValue() returns light level in analog value (0-1023).
     * 0 = bright, 1023 = dark (inverted polarity)
     */
    float getValue() const override { return _value; }

    unsigned long getLastReadTime() const override { return _lastReadTime; }
    bool isFresh() const override;

    /**
     * Get light level in analog value (0-1023).
     * Higher value = darker
     */
    uint16_t getLightLevel() const { return (uint16_t)_value; }

    /**
     * Check if supplemental lighting is needed (light level below threshold).
     */
    bool needsSupplementalLight() const;

private:
    uint8_t _pin;
    unsigned long _lastUpdateTime = 0;
};

#endif // PHOTORESISTOR_SENSOR_H
