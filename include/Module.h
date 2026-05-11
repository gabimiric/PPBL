#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>

// ============================================================================
// BASE CLASS FOR ALL MODULES
// ============================================================================

class Module
{
public:
    virtual ~Module() {}

    /**
     * Initialize the module.
     * Called after creation. Returns true if successful.
     */
    virtual bool init() = 0;

    /**
     * Check if module is connected/available.
     * Returns true if the module hardware is detected and operational.
     */
    virtual bool isAvailable() = 0;

    /**
     * Update module state (called periodically from main loop).
     * Sensors read values, actuators perform control logic.
     */
    virtual void update() = 0;

    /**
     * Get module status as a string for debugging.
     */
    virtual const char *getName() const = 0;

    /**
     * Get module type identifier.
     */
    virtual uint8_t getModuleType() const = 0;

protected:
    bool _available = false;
};

// Module type identifiers
enum ModuleType : uint8_t
{
    MODULE_SOIL_MOISTURE = 1,
    MODULE_DHT = 2,
    MODULE_LIGHT_SENSOR = 3,
    MODULE_PUMP = 4,
    MODULE_FAN = 5,
    MODULE_LED_LIGHT = 6,
    MODULE_ESP32_CAM = 7
};

#endif // MODULE_H
