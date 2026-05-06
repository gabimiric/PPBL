# Plant Care System - Modular Driver Architecture

This is a completely modular, auto-detecting driver system for an autonomous plant care system based on Arduino Uno/Nano.

## Architecture Overview

The system is built on a hierarchy of classes:

```
Module (Base)
├── Sensor
│   ├── SoilMoistureSensor
│   ├── DHTSensor
│   └── BH1750Sensor
└── Actuator
    ├── WaterPump
    ├── VentilationFan
    ├── LEDGrowLight
    └── ESP32CAM (special: can be both sensor and actuator)
```

## Features

✅ **Modular & Configurable**: Enable/disable modules at compile time in `config.h`  
✅ **Auto-Detection**: System detects connected modules on startup  
✅ **Safe Defaults**: Missing modules don't break the system; control logic skips unavailable modules  
✅ **Debug Output**: Comprehensive serial logging for troubleshooting  
✅ **Extensible**: Easy to add new sensors/actuators by implementing base classes  
✅ **Safety Timeouts**: Built-in protection against runaway actuators

## Project Structure

```
PPBL/
├── include/
│   ├── config.h                 # Main configuration (pins, thresholds)
│   ├── Module.h                 # Base class for all modules
│   ├── Sensor.h                 # Base class for sensors
│   ├── Actuator.h               # Base class for actuators
│   ├── SoilMoistureSensor.h      # Capacitive moisture sensor
│   ├── DHTSensor.h              # Temperature & humidity (DHT22/DHT11)
│   ├── BH1750Sensor.h           # Digital light sensor (I2C)
│   ├── WaterPump.h              # Submersible pump control
│   ├── VentilationFan.h         # Ventilation fan control
│   ├── LEDGrowLight.h           # LED grow light with PWM
│   ├── ESP32CAM.h               # Camera module with WiFi
│   └── ModuleManager.h          # Centralized module management & detection
├── src/
│   ├── main.cpp                 # Main application logic & control loops
│   ├── SoilMoistureSensor.cpp
│   ├── DHTSensor.cpp
│   ├── BH1750Sensor.cpp
│   ├── WaterPump.cpp
│   ├── VentilationFan.cpp
│   ├── LEDGrowLight.cpp
│   ├── ESP32CAM.cpp
│   └── ModuleManager.cpp
├── platformio.ini               # PlatformIO configuration
└── README.md                    # This file
```

## Quick Start

### 1. Configure Your Modules

Edit `include/config.h` to match your hardware setup:

```cpp
// Enable/disable modules (1 = enabled, 0 = disabled)
#define ENABLE_SOIL_MOISTURE_SENSOR 1
#define ENABLE_DHT_SENSOR           1
#define ENABLE_LIGHT_SENSOR         1
#define ENABLE_PUMP                 1
#define ENABLE_FAN                  1
#define ENABLE_LED_GROW_LIGHT       1
#define ENABLE_ESP32_CAM            1

// Set pins to match your wiring
#define SOIL_MOISTURE_PIN           A0
#define DHT_SENSOR_PIN              2
#define PUMP_RELAY_PIN              3
#define FAN_RELAY_PIN               4
#define LED_LIGHT_PIN               5
```

Don't have a sensor? Just set `ENABLE_xxx` to `0` and the system will skip it entirely.

### 2. Build & Upload

```bash
pio run -t upload -e uno_r4_wifi
```

### 3. Monitor Serial Output

```bash
pio device monitor -b 9600
```

You'll see:

- Module initialization status
- Real-time sensor readings
- Actuator state changes
- System diagnostics

## Usage Examples

### Reading Sensor Data

```cpp
ModuleManager& manager = ModuleManager::getInstance();

// Get soil moisture
if (manager.isModuleAvailable(MODULE_SOIL_MOISTURE)) {
    SoilMoistureSensor* moisture =
        (SoilMoistureSensor*)manager.getSensor(MODULE_SOIL_MOISTURE);

    float percent = moisture->getMoisturePercentage();
    bool isDry = moisture->isDry();
}

// Get temperature & humidity
if (manager.isModuleAvailable(MODULE_DHT)) {
    DHTSensor* dht = (DHTSensor*)manager.getSensor(MODULE_DHT);

    float temp = dht->getTemperature();
    float humidity = dht->getHumidity();
}

// Get light level
if (manager.isModuleAvailable(MODULE_LIGHT_SENSOR)) {
    BH1750Sensor* light =
        (BH1750Sensor*)manager.getSensor(MODULE_LIGHT_SENSOR);

    float lux = light->getLux();
}
```

### Controlling Actuators

```cpp
ModuleManager& manager = ModuleManager::getInstance();

// Control pump
if (manager.isModuleAvailable(MODULE_PUMP)) {
    WaterPump* pump = (WaterPump*)manager.getActuator(MODULE_PUMP);

    pump->on();        // Turn on
    pump->off();       // Turn off
    pump->toggle();    // Toggle state

    // Check safety timeout
    if (pump->isTimeoutActive()) {
        pump->off();   // Force off if running too long
    }
}

// Control fan
if (manager.isModuleAvailable(MODULE_FAN)) {
    VentilationFan* fan = (VentilationFan*)manager.getActuator(MODULE_FAN);
    fan->on();
    fan->off();
}

// Control LED with brightness
if (manager.isModuleAvailable(MODULE_LED_LIGHT)) {
    LEDGrowLight* led = (LEDGrowLight*)manager.getActuator(MODULE_LED_LIGHT);

    led->on();
    led->setBrightness(128);  // 50% brightness (0-255)
    led->off();
}

// Control camera
if (manager.isModuleAvailable(MODULE_ESP32_CAM)) {
    ESP32CAM* cam = (ESP32CAM*)manager.getModule(MODULE_ESP32_CAM);

    cam->captureImage();                    // Single capture
    cam->setAutoCapInterval(600000);        // Auto-capture every 10 min
}
```

## Configuration Guide

### Soil Moisture Sensor

```cpp
#define SOIL_MOISTURE_DRY_THRESHOLD   300    // Trigger irrigation
#define SOIL_MOISTURE_WET_THRESHOLD   600    // Stop irrigation
```

Calibrate by:

1. Place sensor in dry soil, note ADC reading
2. Place sensor in wet soil, note ADC reading
3. Update thresholds in `config.h`

### DHT Temperature/Humidity

```cpp
#define HUMIDITY_UPPER_THRESHOLD      70    // % RH to trigger fan
#define TEMPERATURE_UPPER_THRESHOLD   28    // °C to trigger fan
#define TEMPERATURE_LOWER_THRESHOLD   15    // °C minimum for operation
```

### Light Sensor (BH1750)

```cpp
#define LIGHT_LEVEL_THRESHOLD         500   // Lux threshold for grow light
#define LED_ON_HOURS                  16    // Hours per day (photoperiod)
#define LED_OFF_HOURS                 8     // Hours off
```

**Note**: Current photoperiod is based on device uptime. For accurate time-based control, integrate an RTC module (DS3231).

### Safety Timeouts

```cpp
#define PUMP_MAX_ON_TIME              60000  // 60 seconds max
#define FAN_MAX_ON_TIME               300000 // 5 minutes max
```

These prevent damage from stuck relays. Adjust based on your system's needs.

## How Auto-Detection Works

On startup, `ModuleManager::initializeAll()`:

1. **Creates** all enabled modules (based on `config.h`)
2. **Calls init()** on each module
3. **Calls isAvailable()** to detect if hardware is actually present
4. **Logs** which modules passed/failed detection
5. **Disables** unavailable modules in control logic

In your main control loop, always check `isModuleAvailable()` before use:

```cpp
if (manager.isModuleAvailable(MODULE_SOIL_MOISTURE)) {
    // Safe to use - module is present and initialized
}
```

## Adding New Modules

To add a new sensor or actuator:

1. **Create header file** (e.g., `include/MyNewSensor.h`)
2. **Inherit from `Sensor` or `Actuator`**
3. **Implement required methods**:
   - `bool init()`
   - `bool isAvailable()`
   - `void update()`
   - `const char* getName()`
   - `uint8_t getModuleType()`
4. **Create implementation** (`.cpp` file)
5. **Register in `ModuleManager::createEnabledModules()`**

Example template:

```cpp
// include/MyNewSensor.h
#include "Sensor.h"

class MyNewSensor : public Sensor {
public:
    bool init() override;
    bool isAvailable() override;
    void update() override;
    const char* getName() const override { return "My New Sensor"; }
    uint8_t getModuleType() const override { return 99; }  // Unique ID
    float getValue() const override { return _value; }

private:
    float _value = 0.0f;
};
```

## Hardware Connections (Arduino Uno)

| Component     | Pin(s) | Type            |
| ------------- | ------ | --------------- |
| Soil Moisture | A0     | Analog Input    |
| DHT22         | 2      | Digital Signal  |
| BH1750        | A4/A5  | I2C (SDA/SCL)   |
| Pump Relay    | 3      | Digital Output  |
| Fan Relay     | 4      | Digital Output  |
| LED Light PWM | 5      | PWM Output (D~) |
| ESP32-CAM     | 8/9    | Serial (RX/TX)  |

**Note**: Pins 3, 5, 6, 9, 10, 11 support PWM on Uno

## Debugging Tips

### Enable Debug Output

```cpp
#define DEBUG_ENABLED 1  // in config.h
```

### Check Module Status

```cpp
ModuleManager& manager = ModuleManager::getInstance();
manager.printStatus();  // Prints all connected modules
```

### Monitor Individual Module

```cpp
Module* module = manager.getModuleAt(0);
if (module) {
    Serial.println(module->getName());
    Serial.println(module->isAvailable() ? "ONLINE" : "OFFLINE");
}
```

### Serial Troubleshooting

Check these messages in serial output:

- `[xxx] Initialized on pin Y` → Module found and initialized
- `[xxx] OFFLINE` → Module configured but not detected
- `[xxx] TIMEOUT` → Actuator safety timeout triggered
- `[xxx] Read failed` → Sensor read error (timing, I2C, etc.)

## Power Considerations

- **Arduino Uno**: Powered via USB or 7-12V adapter, max 500mA

### High-Current Components

- **Water Pump (3-6V)**: Use separate 5V relay + external power supply
- **Fan (5V)**: Use relay module for switching
- **LED Grow Light**: Use PWM to limit current, consider separate power

**Recommendation**: Use a power management board with separate supplies for high-current devices.

## Next Steps

1. **Configure** `config.h` for your hardware
2. **Build & Upload** the firmware
3. **Monitor** serial output to verify all modules
4. **Customize** `updateControlLogic()` in `main.cpp` for your plant's needs
5. **Test** each module manually before final deployment

---

**Happy planting! 🌱**
