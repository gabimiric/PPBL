#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>
#include "Module.h"
#include "Sensor.h"
#include "Actuator.h"

// Maximum number of modules that can be managed
#define MAX_MODULES 16

// ============================================================================
// MODULE MANAGER - CENTRALIZED MODULE DETECTION & CONTROL
// ============================================================================

class ModuleManager
{
public:
    /**
     * Singleton instance getter.
     */
    static ModuleManager &getInstance();

    /**
     * Initialize all configured modules.
     * Detects which modules are connected and prepares them.
     * Call this in setup().
     */
    bool initializeAll();

    /**
     * Update all active modules (read sensors, perform control logic).
     * Call this in loop().
     */
    void updateAll();

    /**
     * Register a module to be managed.
     * Returns true if successfully registered.
     */
    bool registerModule(Module *module);

    /**
     * Get a module by its type.
     * Returns nullptr if module not found or not available.
     */
    Module *getModule(ModuleType type);

    /**
     * Get a sensor by type (cast from getModule).
     */
    Sensor *getSensor(ModuleType type);

    /**
     * Get an actuator by type (cast from getModule).
     */
    Actuator *getActuator(ModuleType type);

    /**
     * Get total number of registered modules.
     */
    uint8_t getModuleCount() const { return _moduleCount; }

    /**
     * Get number of available (connected) modules.
     */
    uint8_t getAvailableCount() const;

    /**
     * Print status of all modules to serial.
     */
    void printStatus() const;

    /**
     * Get module by index (0 to getModuleCount()-1).
     */
    Module *getModuleAt(uint8_t index) const;

    /**
     * Check if a specific module type is available.
     */
    bool isModuleAvailable(ModuleType type) const;

private:
    // Singleton pattern
    ModuleManager() = default;
    ModuleManager(const ModuleManager &) = delete;
    ModuleManager &operator=(const ModuleManager &) = delete;

    // Module storage
    Module *_modules[MAX_MODULES] = {nullptr};
    uint8_t _moduleCount = 0;

    /**
     * Automatically create and register all enabled modules.
     */
    void createEnabledModules();
};

#endif // MODULE_MANAGER_H
