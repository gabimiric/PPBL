#include "Settings.h"
#include <EEPROM.h>

namespace
{
// On-flash layout. Bumping VERSION invalidates older blobs (load() falls back
// to compile-time defaults). The checksum guards against partially-written
// blobs and uninitialised flash (which reads as 0xFF on UNO R4).
struct Blob
{
    uint32_t magic;       // 'GPOD'
    uint8_t version;
    uint8_t mode;
    uint16_t soilDry;
    uint16_t soilWet;
    uint8_t humHigh;
    uint8_t tempHigh;
    uint16_t lightLow;
    uint16_t checksum;
};

constexpr uint32_t MAGIC = 0x47504F44; // 'GPOD'
constexpr uint8_t VERSION = 1;
constexpr int EEPROM_ADDR = 0;
// 5-second debounce: rapid slider drags coalesce into a single flash write,
// keeping us well under the data-flash endurance budget.
constexpr unsigned long DEBOUNCE_MS = 5000;

uint16_t computeChecksum(const Blob &b)
{
    const uint8_t *p = (const uint8_t *)&b;
    uint16_t sum = 0;
    size_t n = sizeof(Blob) - sizeof(b.checksum);
    for (size_t i = 0; i < n; i++)
    {
        sum = (uint16_t)((sum << 1) ^ p[i]);
    }
    return sum;
}
}

void Settings::load()
{
    Blob b;
    EEPROM.get(EEPROM_ADDR, b);

    if (b.magic != MAGIC || b.version != VERSION)
    {
        if (DEBUG_ENABLED)
            Serial.println("[Settings] EEPROM empty/older version — using defaults");
        return;
    }

    if (b.checksum != computeChecksum(b))
    {
        if (DEBUG_ENABLED)
            Serial.println("[Settings] EEPROM checksum mismatch — using defaults");
        return;
    }

    mode = (b.mode <= 1) ? (ControlMode)b.mode : MODE_AUTO;
    soilDryThreshold = b.soilDry;
    soilWetThreshold = b.soilWet;
    humidityHigh = b.humHigh;
    tempHigh = b.tempHigh;
    lightLow = b.lightLow;

    if (DEBUG_ENABLED)
    {
        Serial.print("[Settings] Loaded from EEPROM (mode=");
        Serial.print(mode == MODE_AUTO ? "auto" : "manual");
        Serial.print(", soilDry=");
        Serial.print(soilDryThreshold);
        Serial.print(", soilWet=");
        Serial.print(soilWetThreshold);
        Serial.print(", humHigh=");
        Serial.print(humidityHigh);
        Serial.print(", tempHigh=");
        Serial.print(tempHigh);
        Serial.print(", lightLow=");
        Serial.print(lightLow);
        Serial.println(")");
    }
}

void Settings::markDirty()
{
    _dirty = true;
    _dirtySince = millis();
}

void Settings::flushIfDue()
{
    if (!_dirty) return;
    if (millis() - _dirtySince < DEBOUNCE_MS) return;
    writeNow();
    _dirty = false;
}

void Settings::writeNow()
{
    Blob b{};
    b.magic = MAGIC;
    b.version = VERSION;
    b.mode = (uint8_t)mode;
    b.soilDry = soilDryThreshold;
    b.soilWet = soilWetThreshold;
    b.humHigh = humidityHigh;
    b.tempHigh = tempHigh;
    b.lightLow = lightLow;
    b.checksum = computeChecksum(b);
    EEPROM.put(EEPROM_ADDR, b);

    if (DEBUG_ENABLED)
        Serial.println("[Settings] Flushed to EEPROM");
}
