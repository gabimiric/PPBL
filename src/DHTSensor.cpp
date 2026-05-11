#include "DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type)
    : _pin(pin), _type(type), _dht(pin, mapType(type)) {}

bool DHTSensor::init()
{
    _dht.begin();
    // First reading takes ~1 s on a cold sensor. Give it a probe + retry so we
    // don't mark the module unavailable just because the very first read happened
    // before the sensor stabilized.
    delay(1100);
    bool ok = false;
    for (uint8_t i = 0; i < 3 && !ok; i++)
    {
        ok = readDHT();
        if (!ok) delay(300);
    }

    _available = ok;

    if (DEBUG_ENABLED)
    {
        if (ok)
        {
            Serial.print("[DHTSensor] Initialized DHT");
            Serial.print(_type);
            Serial.print(" on pin ");
            Serial.println(_pin);
        }
        else
        {
            Serial.print("[DHTSensor] FAILED to detect DHT");
            Serial.print(_type);
            Serial.print(" on pin ");
            Serial.println(_pin);
        }
    }

    return ok;
}

bool DHTSensor::isAvailable()
{
    return _available;
}

void DHTSensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime < DHT_READ_INTERVAL)
    {
        return;
    }

    if (readDHT())
    {
        _lastReadTime = now;

        if (DEBUG_ENABLED)
        {
            Serial.print("[DHTSensor] Temp: ");
            Serial.print(_temperature);
            Serial.print("°C | Humidity: ");
            Serial.print(_humidity);
            Serial.println("% RH");
        }
    }
    else if (DEBUG_ENABLED)
    {
        Serial.println("[DHTSensor] Read failed!");
    }

    _lastUpdateTime = now;
}

bool DHTSensor::isFresh() const
{
    return (millis() - _lastReadTime) < DHT_READ_INTERVAL * 2;
}

bool DHTSensor::isHumidityHigh() const
{
    return _humidity > HUMIDITY_UPPER_THRESHOLD;
}

bool DHTSensor::isTemperatureHigh() const
{
    return _temperature > TEMPERATURE_UPPER_THRESHOLD;
}

bool DHTSensor::isTemperatureLow() const
{
    return _temperature < TEMPERATURE_LOWER_THRESHOLD;
}

bool DHTSensor::readDHT()
{
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();
    if (isnan(t) || isnan(h)) return false;
    _temperature = t;
    _humidity = h;
    _value = t;
    return true;
}
