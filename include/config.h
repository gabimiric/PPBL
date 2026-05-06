#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// MODULE AVAILABILITY CONFIGURATION
// ============================================================================
// Set each to 1 to ENABLE or 0 to DISABLE the module during compilation
// Even if disabled here, modules can be detected at runtime if present

#define ENABLE_SOIL_MOISTURE_SENSOR 1
#define ENABLE_DHT_SENSOR 1
#define ENABLE_LIGHT_SENSOR 1
#define ENABLE_PUMP 1
#define ENABLE_FAN 1
#define ENABLE_LED_GROW_LIGHT 1 // Will auto-detect if hardware is present during init()
#define ENABLE_ESP32_CAM 1

// ============================================================================
// LIGHT SENSOR TYPE SELECTION
// ============================================================================
// Choose ONE: Set to 1 to use that sensor, 0 to disable
#define USE_BH1750_LIGHT_SENSOR 0        // I2C digital light sensor
#define USE_PHOTORESISTOR_LIGHT_SENSOR 1 // Analog photoresistor with 10k resistor

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

// Analog Pins (Arduino Uno has A0-A5)
#define SOIL_MOISTURE_PIN A0
#define LIGHT_SENSOR_PIN A1

// Digital Pins
#define DHT_SENSOR_PIN 2
#define PUMP_RELAY_PIN 3
#define FAN_HBRIDGE_IN1_PIN 4
#define FAN_HBRIDGE_IN2_PIN 5
#define FAN_HBRIDGE_EN_PIN 6
#define LED_LIGHT_PIN 9
#define DISPLAY_BUTTON_PIN 8
#define ESP32_CAM_RX_PIN 10
#define ESP32_CAM_TX_PIN 11

// I2C Pins (Hardware on Arduino Uno: A4=SDA, A5=SCL)
#define I2C_SDA_PIN A4
#define I2C_SCL_PIN A5

// BH1750 I2C Address (0x23 or 0x5C depending on ADDR pin)
#define BH1750_I2C_ADDR 0x23

// ============================================================================
// SENSOR THRESHOLDS & CONFIGURATION
// ============================================================================

// DHT Sensor Type (11 for DHT11, 22 for DHT22)
#define DHT_SENSOR_TYPE 11

// Soil Moisture (analog 0-1023)
// Calibrate these for your specific sensor. For many probes: dry > wet.
#define SOIL_MOISTURE_DRY_THRESHOLD 20
#define SOIL_MOISTURE_WET_THRESHOLD 30

// DHT Thresholds (same for DHT11 and DHT22)
#define HUMIDITY_UPPER_THRESHOLD 70    // Turn on fan if > 70% RH
#define TEMPERATURE_UPPER_THRESHOLD 28 // Turn on fan if > 28°C
#define TEMPERATURE_LOWER_THRESHOLD 15 // Lower bound for operation

// Light (Lux threshold for supplemental lighting)
#define LIGHT_LEVEL_THRESHOLD 500

// Photoperiod (hours)
#define LED_ON_HOURS 16
#define LED_OFF_HOURS 8

// ============================================================================
// COMMUNICATION TIMEOUTS & INTERVALS
// ============================================================================

// DHT sensor reading delay (ms)
#define DHT_READ_INTERVAL 2000

// Light sensor reading delay (ms)
#define LIGHT_READ_INTERVAL 1000

// Soil moisture update interval (ms)
#define SOIL_MOISTURE_READ_INTERVAL 1000

// Pump safety cutoff (max on time in ms)
#define PUMP_MAX_ON_TIME 60000 // 60 seconds

// Fan safety cutoff (max on time in ms)
#define FAN_MAX_ON_TIME 300000 // 5 minutes

// ============================================================================
// DEBUG & SERIAL COMMUNICATION
// ============================================================================

#define DEBUG_ENABLED 1
#define SERIAL_BAUD_RATE 9600

#endif // CONFIG_H
