#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "Sensor.h"
#include "config.h"
#include <DHT.h>

// ============================================================================
// DHT22/DHT11 TEMPERATURE & HUMIDITY SENSOR DRIVER
// Wraps Adafruit DHT library so timing is handled by an interrupt-safe
// implementation (custom bit-bang was unreliable on the 48 MHz UNO R4).
// ============================================================================

class DHTSensor : public Sensor
{
public:
    DHTSensor(uint8_t pin = DHT_SENSOR_PIN, uint8_t type = DHT_SENSOR_TYPE);
    virtual ~DHTSensor() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "DHT Sensor"; }
    uint8_t getModuleType() const override { return MODULE_DHT; }

    float getValue() const override { return _temperature; }
    unsigned long getLastReadTime() const override { return _lastReadTime; }
    bool isFresh() const override;

    float getTemperature() const { return _temperature; }
    float getHumidity() const { return _humidity; }

    bool isHumidityHigh() const;
    bool isTemperatureHigh() const;
    bool isTemperatureLow() const;

private:
    uint8_t _pin;
    uint8_t _type; // 22 for DHT22, 11 for DHT11
    DHT _dht;
    float _temperature = 0.0f;
    float _humidity = 0.0f;
    unsigned long _lastUpdateTime = 0;

    // Resolve module's numeric type to the library's DHT* constant.
    static uint8_t mapType(uint8_t type) { return type == 22 ? DHT22 : DHT11; }

    bool readDHT();
};

#endif // DHT_SENSOR_H
