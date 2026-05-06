#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "Sensor.h"
#include "config.h"

// ============================================================================
// DHT22/DHT11 TEMPERATURE & HUMIDITY SENSOR DRIVER
// ============================================================================

class DHTSensor : public Sensor
{
public:
    DHTSensor(uint8_t pin = DHT_SENSOR_PIN, uint8_t type = 22);
    virtual ~DHTSensor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "DHT Sensor"; }
    uint8_t getModuleType() const override { return MODULE_DHT; }

    /**
     * getValue() returns temperature (in °C).
     */
    float getValue() const override { return _temperature; }

    unsigned long getLastReadTime() const override { return _lastReadTime; }
    bool isFresh() const override;

    /**
     * Get current temperature in Celsius.
     */
    float getTemperature() const { return _temperature; }

    /**
     * Get current relative humidity (0-100%).
     */
    float getHumidity() const { return _humidity; }

    /**
     * Check if humidity is too high (needs ventilation).
     */
    bool isHumidityHigh() const;

    /**
     * Check if temperature is too high (needs cooling).
     */
    bool isTemperatureHigh() const;

    /**
     * Check if temperature is too low.
     */
    bool isTemperatureLow() const;

private:
    uint8_t _pin;
    uint8_t _type; // 22 for DHT22, 11 for DHT11
    float _temperature = 0.0f;
    float _humidity = 0.0f;
    unsigned long _lastUpdateTime = 0;

    /**
     * Read data from DHT sensor.
     * Returns true if read was successful.
     */
    bool readDHT();
};

#endif // DHT_SENSOR_H
