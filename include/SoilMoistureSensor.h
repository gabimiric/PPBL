#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

#include "Sensor.h"
#include "config.h"

// ============================================================================
// SOIL MOISTURE SENSOR DRIVER (Capacitive)
// ============================================================================

class SoilMoistureSensor : public Sensor
{
public:
    SoilMoistureSensor(uint8_t pin = SOIL_MOISTURE_PIN);
    virtual ~SoilMoistureSensor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "Soil Moisture Sensor"; }
    uint8_t getModuleType() const override { return MODULE_SOIL_MOISTURE; }

    float getValue() const override { return _value; }
    unsigned long getLastReadTime() const override { return _lastReadTime; }
    bool isFresh() const override;

    /**
     * Get moisture level as percentage (0-100%).
     * Uses dry and wet thresholds from config.h
     */
    float getMoisturePercentage() const;

    /**
     * Read raw ADC value (0-1023).
     */
    uint16_t getRawValue() const { return _rawValue; }

    /**
     * Check if soil is dry and needs watering.
     */
    bool isDry() const;

    /**
     * Check if soil is sufficiently moist.
     */
    bool isWet() const;

private:
    uint8_t _pin;
    uint16_t _rawValue = 0;
    unsigned long _lastUpdateTime = 0;
};

#endif // SOIL_MOISTURE_SENSOR_H
