#include "ModuleManager.h"
#include "SoilMoistureSensor.h"
#include "DHTSensor.h"
#include "BH1750Sensor.h"
#include "PhotoresistorSensor.h"
#include "WaterPump.h"
#include "VentilationFan.h"
#include "LEDGrowLight.h"
#include "ESP32CAM.h"

ModuleManager &ModuleManager::getInstance()
{
    static ModuleManager instance;
    return instance;
}

bool ModuleManager::initializeAll()
{
    // Create all enabled modules
    createEnabledModules();

    // Initialize each module
    uint8_t successCount = 0;
    uint8_t failureCount = 0;

    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i])
        {
            if (_modules[i]->init())
            {
                successCount++;

                if (DEBUG_ENABLED)
                {
                    Serial.print("[ModuleManager] ✓ ");
                    Serial.print(_modules[i]->getName());
                    Serial.println(" initialized");
                }
            }
            else
            {
                failureCount++;

                if (DEBUG_ENABLED)
                {
                    Serial.print("[ModuleManager] ✗ ");
                    Serial.print(_modules[i]->getName());
                    Serial.println(" initialization failed");
                }
            }
        }
    }

    if (DEBUG_ENABLED)
    {
        Serial.print("[ModuleManager] Initialization complete: ");
        Serial.print(successCount);
        Serial.print(" successful, ");
        Serial.print(failureCount);
        Serial.println(" failed");
    }

    return failureCount == 0;
}

void ModuleManager::updateAll()
{
    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i] && _modules[i]->isAvailable())
        {
            _modules[i]->update();
        }
    }
}

bool ModuleManager::registerModule(Module *module)
{
    if (_moduleCount >= MAX_MODULES || module == nullptr)
    {
        return false;
    }

    _modules[_moduleCount++] = module;
    return true;
}

Module *ModuleManager::getModule(ModuleType type)
{
    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i] && _modules[i]->isAvailable() &&
            _modules[i]->getModuleType() == type)
        {
            return _modules[i];
        }
    }
    return nullptr;
}

Sensor *ModuleManager::getSensor(ModuleType type)
{
    Module *module = getModule(type);
    return module ? dynamic_cast<Sensor *>(module) : nullptr;
}

Actuator *ModuleManager::getActuator(ModuleType type)
{
    Module *module = getModule(type);
    return module ? dynamic_cast<Actuator *>(module) : nullptr;
}

uint8_t ModuleManager::getAvailableCount() const
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i] && _modules[i]->isAvailable())
        {
            count++;
        }
    }
    return count;
}

void ModuleManager::printStatus() const
{
    Serial.println("\n========== MODULE STATUS ==========");

    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i])
        {
            Serial.print("[");
            Serial.print(i + 1);
            Serial.print("] ");
            Serial.print(_modules[i]->getName());
            Serial.print(" - ");
            Serial.println(_modules[i]->isAvailable() ? "ONLINE" : "OFFLINE");
        }
    }

    Serial.print("\nTotal: ");
    Serial.print(_moduleCount);
    Serial.print(" modules, ");
    Serial.print(getAvailableCount());
    Serial.println(" available");
    Serial.println("====================================\n");
}

Module *ModuleManager::getModuleAt(uint8_t index) const
{
    if (index >= _moduleCount)
    {
        return nullptr;
    }
    return _modules[index];
}

bool ModuleManager::isModuleAvailable(ModuleType type) const
{
    for (uint8_t i = 0; i < _moduleCount; i++)
    {
        if (_modules[i] && _modules[i]->getModuleType() == type &&
            _modules[i]->isAvailable())
        {
            return true;
        }
    }
    return false;
}

void ModuleManager::createEnabledModules()
{
#if ENABLE_SOIL_MOISTURE_SENSOR
    registerModule(new SoilMoistureSensor(SOIL_MOISTURE_PIN));
#endif

#if ENABLE_DHT_SENSOR
    registerModule(new DHTSensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE));
#endif

#if ENABLE_LIGHT_SENSOR
#if USE_BH1750_LIGHT_SENSOR
    registerModule(new BH1750Sensor(BH1750_I2C_ADDR));
#elif USE_PHOTORESISTOR_LIGHT_SENSOR
    registerModule(new PhotoresistorSensor(LIGHT_SENSOR_PIN));
#endif
#endif

#if ENABLE_PUMP
    registerModule(new WaterPump(PUMP_RELAY_PIN));
#endif

#if ENABLE_FAN
    registerModule(new VentilationFan(FAN_HBRIDGE_IN1_PIN, FAN_HBRIDGE_IN2_PIN, FAN_HBRIDGE_EN_PIN));
#endif

#if ENABLE_LED_GROW_LIGHT
    registerModule(new LEDGrowLight(LED_LIGHT_PIN));
#endif

#if ENABLE_ESP32_CAM
    registerModule(new ESP32CAM(ESP32_CAM_RX_PIN, ESP32_CAM_TX_PIN));
#endif
}
