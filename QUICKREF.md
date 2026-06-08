# Quick Reference Guide - Module Operations

## System Initialization

```cpp
#include "ModuleManager.h"

void setup() {
    Serial.begin(9600);
    ModuleManager::getInstance().initializeAll();
}

void loop() {
    ModuleManager::getInstance().updateAll();
}
```

## Sensor Data Access

### Soil Moisture Sensor

```cpp
ModuleManager& mgr = ModuleManager::getInstance();

if (mgr.isModuleAvailable(MODULE_SOIL_MOISTURE)) {
    auto moisture = (SoilMoistureSensor*)mgr.getSensor(MODULE_SOIL_MOISTURE);

    float percentage = moisture->getMoisturePercentage();  // 0-100%
    uint16_t rawValue = moisture->getRawValue();          // 0-1023
    bool isDry = moisture->isDry();
    bool isWet = moisture->isWet();

    if (moisture->isFresh()) {
        // Reading is up-to-date
    }
}
```

### DHT Temperature/Humidity

```cpp
if (mgr.isModuleAvailable(MODULE_DHT)) {
    auto dht = (DHTSensor*)mgr.getSensor(MODULE_DHT);

    float tempC = dht->getTemperature();                  // Celsius
    float humRH = dht->getHumidity();                     // %RH

    bool tooHot = dht->isTemperatureHigh();
    bool tooCold = dht->isTemperatureLow();
    bool tooHumid = dht->isHumidityHigh();
}
```

### Light Sensor (BH1750)

```cpp
if (mgr.isModuleAvailable(MODULE_LIGHT_SENSOR)) {
    auto bh = (BH1750Sensor*)mgr.getSensor(MODULE_LIGHT_SENSOR);

    float lux = bh->getLux();                             // 1-65535
    bool needsLight = bh->needsSupplementalLight();
}
```

## Actuator Control

### Water Pump

```cpp
if (mgr.isModuleAvailable(MODULE_PUMP)) {
    auto pump = (WaterPump*)mgr.getActuator(MODULE_PUMP);

    pump->on();                                           // Turn on
    pump->off();                                          // Turn off
    pump->toggle();                                       // Toggle state

    bool isRunning = pump->isOn();
    unsigned long runTime = pump->getRunTime();           // Milliseconds
    bool timeout = pump->isTimeoutActive();               // Safety check
    pump->resetRunTime();
}
```

### Ventilation Fan

```cpp
if (mgr.isModuleAvailable(MODULE_FAN)) {
    auto fan = (VentilationFan*)mgr.getActuator(MODULE_FAN);

    fan->on();
    fan->off();
    fan->toggle();

    bool isRunning = fan->isOn();
    unsigned long runTime = fan->getRunTime();
}
```

### LED Grow Light (with PWM)

```cpp
if (mgr.isModuleAvailable(MODULE_LED_LIGHT)) {
    auto led = (LEDGrowLight*)mgr.getActuator(MODULE_LED_LIGHT);

    led->on();                                            // Full brightness
    led->off();
    led->toggle();

    led->setBrightness(128);                              // 0-255 scale
    led->setBrightness(255);                              // 100%
    led->setBrightness(64);                               // 25%

    uint8_t brightness = led->getBrightness();
    bool inPhotoperiod = led->isInPhotoperiod();
}
```

### Camera (ESP32-CAM)

```cpp
if (mgr.isModuleAvailable(MODULE_ESP32_CAM)) {
    auto cam = (ESP32CAM*)mgr.getModule(MODULE_ESP32_CAM);

    // One-time capture
    cam->captureImage();

    // Auto time-lapse (every 10 minutes)
    cam->setAutoCapInterval(600000);

    // Disable auto-capture
    cam->setAutoCapInterval(0);

    bool ready = cam->isReady();
    uint32_t count = cam->getImageCount();
}
```

## Control Logic Examples

### Example 1: Simple Watering Logic

```cpp
void checkWatering() {
    ModuleManager& mgr = ModuleManager::getInstance();

    if (mgr.isModuleAvailable(MODULE_SOIL_MOISTURE) &&
        mgr.isModuleAvailable(MODULE_PUMP)) {

        auto moisture = (SoilMoistureSensor*)mgr.getSensor(MODULE_SOIL_MOISTURE);
        auto pump = (WaterPump*)mgr.getActuator(MODULE_PUMP);

        if (moisture->isDry()) {
            pump->on();
        } else if (moisture->isWet()) {
            pump->off();
        }
    }
}
```

### Example 2: Climate Control

```cpp
void checkClimate() {
    ModuleManager& mgr = ModuleManager::getInstance();

    if (mgr.isModuleAvailable(MODULE_DHT) &&
        mgr.isModuleAvailable(MODULE_FAN)) {

        auto dht = (DHTSensor*)mgr.getSensor(MODULE_DHT);
        auto fan = (VentilationFan*)mgr.getActuator(MODULE_FAN);

        // Turn on if too hot or too humid
        if (dht->isTemperatureHigh() || dht->isHumidityHigh()) {
            fan->on();
        } else {
            fan->off();
        }
    }
}
```

### Example 3: Smart Lighting

```cpp
void checkLighting() {
    ModuleManager& mgr = ModuleManager::getInstance();

    if (mgr.isModuleAvailable(MODULE_LIGHT_SENSOR) &&
        mgr.isModuleAvailable(MODULE_LED_LIGHT)) {

        auto light = (BH1750Sensor*)mgr.getSensor(MODULE_LIGHT_SENSOR);
        auto led = (LEDGrowLight*)mgr.getActuator(MODULE_LED_LIGHT);

        if (light->needsSupplementalLight()) {
            led->on();
            led->setBrightness(200);  // 78% brightness
        } else {
            led->off();
        }
    }
}
```

### Example 4: Integrated System Check

```cpp
void systemHealthCheck() {
    ModuleManager& mgr = ModuleManager::getInstance();

    Serial.print("Active modules: ");
    Serial.println(mgr.getAvailableCount());

    for (uint8_t i = 0; i < mgr.getModuleCount(); i++) {
        Module* mod = mgr.getModuleAt(i);
        if (mod) {
            Serial.print("  - ");
            Serial.print(mod->getName());
            Serial.println(mod->isAvailable() ? " [OK]" : " [OFFLINE]");
        }
    }
}
```

## Debugging & Serial Output

### Print Full Status

```cpp
ModuleManager::getInstance().printStatus();
```

Output:

```
========== SYSTEM STATUS ==========
Uptime: 120 seconds
  Soil Moisture: 65.3% (Raw: 450)
  Temperature: 24.5°C | Humidity: 58.2% RH
  Light Level: 350 lux
  Pump: OFF
  Fan: OFF
  LED Light: OFF (Brightness: 0%)
  Camera: READY | Images captured: 12
===================================
```

### Monitor Single Module Type

```cpp
if (ModuleManager::getInstance().isModuleAvailable(MODULE_DHT)) {
    Serial.println("DHT sensor is connected and ready");
} else {
    Serial.println("DHT sensor not found or offline");
}
```

### Check Module Availability at Runtime

```cpp
ModuleManager& mgr = ModuleManager::getInstance();

// Check before using
if (!mgr.isModuleAvailable(MODULE_PUMP)) {
    Serial.println("Warning: Pump not available, skipping irrigation");
    return;
}

// Safe to proceed with pump operations
```

## Pin Configuration Quick Reference

```cpp
// Analog Inputs
SOIL_MOISTURE_PIN           A0

// Digital Outputs (Relays)
PUMP_RELAY_PIN              3
FAN_RELAY_PIN               4

// PWM Outputs (0-255 control)
LED_LIGHT_PIN               5

// Serial/I2C
DHT_SENSOR_PIN              2
BH1750_I2C_ADDR             0x23  // I2C address on A4/A5
ESP32_CAM_RX_PIN            8
ESP32_CAM_TX_PIN            9
```

## Common Threshold Values

```cpp
// Soil Moisture (0-1023 range)
SOIL_MOISTURE_DRY_THRESHOLD       300
SOIL_MOISTURE_WET_THRESHOLD       600

// Temperature (Celsius)
HUMIDITY_UPPER_THRESHOLD          70    // % RH
TEMPERATURE_UPPER_THRESHOLD       28    // °C
TEMPERATURE_LOWER_THRESHOLD       15    // °C

// Light (Lux)
LIGHT_LEVEL_THRESHOLD             500   // When to use grow light

// Safety Timeouts (Milliseconds)
PUMP_MAX_ON_TIME                  60000  // 60 seconds
FAN_MAX_ON_TIME                   300000 // 5 minutes
```

## Type Casting Reference

```cpp
// Get typed modules
SoilMoistureSensor* sensor1 = (SoilMoistureSensor*)
    mgr.getSensor(MODULE_SOIL_MOISTURE);

DHTSensor* sensor2 = (DHTSensor*)
    mgr.getSensor(MODULE_DHT);

WaterPump* act1 = (WaterPump*)
    mgr.getActuator(MODULE_PUMP);

VentilationFan* act2 = (VentilationFan*)
    mgr.getActuator(MODULE_FAN);

LEDGrowLight* act3 = (LEDGrowLight*)
    mgr.getActuator(MODULE_LED_LIGHT);

ESP32CAM* cam = (ESP32CAM*)
    mgr.getModule(MODULE_ESP32_CAM);
```

## Best Practices

✅ **DO**: Check module availability before using

```cpp
if (mgr.isModuleAvailable(MODULE_PUMP)) {
    // Safe to use
}
```

✅ **DO**: Use sensor freshness checks

```cpp
if (sensor->isFresh()) {
    float value = sensor->getValue();
}
```

✅ **DO**: Monitor actuator runtime

```cpp
if (pump->getRunTime() > PUMP_MAX_ON_TIME) {
    pump->off();
}
```

❌ **DON'T**: Assume modules are connected

```cpp
// BAD - Will crash if module unavailable
auto pump = (WaterPump*)mgr.getActuator(MODULE_PUMP);
pump->on();  // Null pointer!
```

❌ **DON'T**: Poll sensors too frequently

```cpp
// Sensors have built-in update intervals - calling update() more often won't speed up readings
```

---

For more details, see [README.md](README.md)
