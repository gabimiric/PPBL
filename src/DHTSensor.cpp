#include "DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type)
    : _pin(pin), _type(type) {}

bool DHTSensor::init()
{
    pinMode(_pin, INPUT);

    // Try to read the sensor to verify it's connected
    if (!readDHT())
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.print("[DHTSensor] FAILED to detect DHT");
            Serial.print(_type);
            Serial.print(" on pin ");
            Serial.println(_pin);
        }
        return false;
    }

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.print("[DHTSensor] Initialized DHT");
        Serial.print(_type);
        Serial.print(" on pin ");
        Serial.println(_pin);
    }

    return true;
}

bool DHTSensor::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void DHTSensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime < DHT_READ_INTERVAL)
    {
        return; // Too soon to read again
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
    // This is a simplified DHT reading routine.
    // For production, consider using a library like DHT.h from Adafruit

    // Set pin as output and pull low for 18-20ms (start signal)
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(20); // 20ms start signal
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40); // Wait 40us

    // Set pin as input
    pinMode(_pin, INPUT_PULLUP);

    // DHT11 should respond with 80us low pulse then 80us high pulse
    // Use a more robust timeout checking mechanism (max 500us wait)
    uint8_t timeoutCounter = 0;
    while (digitalRead(_pin) == HIGH && timeoutCounter < 50)
    {
        delayMicroseconds(10);
        timeoutCounter++;
    }

    if (timeoutCounter >= 50)
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[DHTSensor] No response from sensor (timeout waiting for low)");
        }
        return false; // No response from sensor
    }

    // Wait for the sensor to pull high before sending data
    timeoutCounter = 0;
    while (digitalRead(_pin) == LOW && timeoutCounter < 50)
    {
        delayMicroseconds(10);
        timeoutCounter++;
    }

    if (timeoutCounter >= 50)
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[DHTSensor] No response from sensor (timeout waiting for high)");
        }
        return false;
    }

    // Read 40 bits (5 bytes)
    uint8_t data[5] = {0, 0, 0, 0, 0};

    for (int byteIdx = 0; byteIdx < 5; byteIdx++)
    {
        for (int bitIdx = 0; bitIdx < 8; bitIdx++)
        {
            // Wait for low pulse
            timeoutCounter = 0;
            while (digitalRead(_pin) == HIGH && timeoutCounter < 50)
            {
                delayMicroseconds(10);
                timeoutCounter++;
            }

            if (timeoutCounter >= 50)
            {
                if (DEBUG_ENABLED)
                {
                    Serial.println("[DHTSensor] Timeout waiting for bit low");
                }
                return false;
            }

            // Wait for high pulse and measure duration
            timeoutCounter = 0;
            while (digitalRead(_pin) == LOW && timeoutCounter < 50)
            {
                delayMicroseconds(10);
                timeoutCounter++;
            }

            // Measure high pulse duration (0 bit: ~26us, 1 bit: ~70us)
            unsigned long highCounter = 0;
            while (digitalRead(_pin) == HIGH && highCounter < 10)
            {
                delayMicroseconds(10);
                highCounter++;
            }

            // If high pulse was longer (~7x10us = 70us), it's a 1 bit
            if (highCounter > 3)
            {
                data[byteIdx] |= (1 << (7 - bitIdx));
            }
        }
    }

    // Verify checksum (sum of first 4 bytes = last byte)
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (checksum != data[4])
    {
        if (DEBUG_ENABLED)
        {
            Serial.print("[DHTSensor] Checksum failed: Expected ");
            Serial.print(checksum);
            Serial.print(" got ");
            Serial.println(data[4]);
        }
        return false;
    }

    // Parse data based on DHT type
    if (_type == 22)
    {
        // DHT22: 16-bit humidity (data[0]:data[1]), 16-bit temp (data[2]:data[3])
        _humidity = ((float)(data[0] << 8 | data[1])) / 10.0f;

        _temperature = ((float)((data[2] & 0x7F) << 8 | data[3])) / 10.0f;
        if (data[2] & 0x80)
        { // Negative temperature
            _temperature *= -1;
        }
    }
    else
    {
        // DHT11: 8-bit humidity (data[0]), 8-bit temp (data[2]), decimal parts in data[1] and data[3]
        _humidity = (float)data[0] + (data[1] / 100.0f);
        _temperature = (float)data[2] + (data[3] / 100.0f);

        // DHT11 temp negative flag
        if (data[2] & 0x80)
        {
            _temperature = -_temperature;
        }
    }

    if (DEBUG_ENABLED)
    {
        Serial.print("[DHTSensor] Raw data: ");
        for (int i = 0; i < 5; i++)
        {
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    return true;
}
