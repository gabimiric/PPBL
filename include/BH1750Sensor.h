#ifndef BH1750_SENSOR_H
#define BH1750_SENSOR_H

#include "Sensor.h"
#include "config.h"

// ============================================================================
// BH1750 DIGITAL LIGHT INTENSITY SENSOR DRIVER
// ============================================================================

class BH1750Sensor : public Sensor
{
public:
    BH1750Sensor(uint8_t i2cAddress = BH1750_I2C_ADDR);
    virtual ~BH1750Sensor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "BH1750 Light Sensor"; }
    uint8_t getModuleType() const override { return MODULE_LIGHT_SENSOR; }

    /**
     * getValue() returns light level in lux.
     */
    float getValue() const override { return _value; }

    unsigned long getLastReadTime() const override { return _lastReadTime; }
    bool isFresh() const override;

    /**
     * Get light level in lux (1-65535).
     */
    float getLux() const { return _value; }

    /**
     * Check if supplemental lighting is needed.
     */
    bool needsSupplementalLight() const;

private:
    uint8_t _i2cAddress;
    unsigned long _lastUpdateTime = 0;

    /**
     * Send command to BH1750 sensor.
     */
    bool sendCommand(uint8_t cmd);

    /**
     * Read two bytes from BH1750 and convert to lux.
     */
    bool readLux();
};

#endif // BH1750_SENSOR_H
