#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "config.h"

// Runtime-mutable configuration. The dashboard can change these values via the
// REST API; the control logic in main.cpp reads from here instead of using the
// compile-time #defines directly so they remain editable at runtime. Mode and
// thresholds are persisted to EEPROM so settings survive a reboot.

enum ControlMode : uint8_t
{
    MODE_AUTO = 0,
    MODE_MANUAL = 1
};

struct Settings
{
    ControlMode mode = MODE_AUTO;

    // Persisted thresholds (mirror the #defines from config.h on first boot,
    // restored from EEPROM on subsequent boots).
    uint16_t soilDryThreshold = SOIL_MOISTURE_DRY_THRESHOLD;
    uint16_t soilWetThreshold = SOIL_MOISTURE_WET_THRESHOLD;
    uint8_t humidityHigh = HUMIDITY_UPPER_THRESHOLD;
    uint8_t tempHigh = TEMPERATURE_UPPER_THRESHOLD;
    uint16_t lightLow = LIGHT_LEVEL_THRESHOLD;

    // Volatile: manual actuator commands (only used when mode == MODE_MANUAL).
    // Not persisted — we don't want a half-finished irrigation command to
    // resume after a power blip.
    bool manualPump = false;
    bool manualFan = false;
    bool manualLed = false;

    // Volatile: in MODE_AUTO, frontend manual commands temporarily override
    // automation until these deadlines pass (millis epoch).
    unsigned long manualPumpOverrideUntil = 0;
    unsigned long manualFanOverrideUntil = 0;
    unsigned long manualLedOverrideUntil = 0;

    // Load persisted settings from EEPROM. Call once in setup().
    void load();
    // Mark settings dirty; flushIfDue() will eventually persist them.
    void markDirty();
    // Persist to EEPROM if dirty and the debounce interval has elapsed. Call
    // from loop() — debouncing protects flash wear from rapid slider drags.
    void flushIfDue();

    static Settings &instance()
    {
        static Settings s;
        return s;
    }

private:
    bool _dirty = false;
    unsigned long _dirtySince = 0;
    void writeNow();
};

#endif // SETTINGS_H
