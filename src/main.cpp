#include <Arduino.h>
#include "config.h"
#include "ModuleManager.h"
#include "DisplayManager.h"
#include "Sensor.h"
#include "Actuator.h"
#include "SoilMoistureSensor.h"
#include "DHTSensor.h"
#include "BH1750Sensor.h"
#include "PhotoresistorSensor.h"
#include "WaterPump.h"
#include "VentilationFan.h"
#include "LEDGrowLight.h"
#include "ESP32CAM.h"

// ============================================================================
// SYSTEM-LEVEL CONTROL LOGIC
// ============================================================================

// Forward declarations
void initializeSystem();
void updateSensors();
void updateControlLogic();
void printSystemStatus();

// State tracking for fan control (avoid repeated on/off)
static bool _fanIsOn = false;

// State tracking for pump control (avoid repeated on/off)
static bool _pumpIsOn = false;

// State tracking for display button
static unsigned long _lastButtonPressTime = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 200;

// ============================================================================
// SETUP - Called once at startup
// ============================================================================

void setup()
{
  // Initialize Serial communication for debugging
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  if (DEBUG_ENABLED)
  {
    Serial.println("\n\n========================================");
    Serial.println("  AUTONOMOUS PLANT CARE SYSTEM");
    Serial.println("  Initializing...");
    Serial.println("========================================\n");
  }

  // Initialize and detect all modules
  initializeSystem();

  // Print initial system status
  if (DEBUG_ENABLED)
  {
    delay(1000);
    printSystemStatus();
  }
}

// ============================================================================
// MAIN LOOP - Called repeatedly
// ============================================================================

void loop()
{
  // Check display button for page switching
  unsigned long now = millis();
  if (digitalRead(DISPLAY_BUTTON_PIN) == LOW) // Button pressed (active LOW)
  {
    if (now - _lastButtonPressTime > BUTTON_DEBOUNCE_MS)
    {
      DisplayManager &display = DisplayManager::getInstance();
      display.togglePage();
      _lastButtonPressTime = now;
    }
  }

  // Update all sensors and modules
  ModuleManager &manager = ModuleManager::getInstance();
  manager.updateAll();

  // Update OLED display (every 100ms)
  DisplayManager &display = DisplayManager::getInstance();
  display.update();

  // Execute control logic based on sensor readings
  updateControlLogic();

  // Optional: Print periodic status updates
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000)
  { // Every 10 seconds
    if (DEBUG_ENABLED)
    {
      printSystemStatus();
    }
    lastStatusPrint = millis();
  }
}

// ============================================================================
// INITIALIZATION: Detect and setup all available modules
// ============================================================================

void initializeSystem()
{
  ModuleManager &manager = ModuleManager::getInstance();
  DisplayManager &display = DisplayManager::getInstance();

  // Initialize display button
  pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP);

  // Initialize OLED display first
  if (!display.init() && DEBUG_ENABLED)
  {
    Serial.println("[Main] Warning: Display initialization failed");
  }

  // Initialize all configured modules
  // This will auto-detect which modules are actually connected
  bool allGood = manager.initializeAll();

  if (DEBUG_ENABLED)
  {
    Serial.print("\nSystem initialization: ");
    Serial.println(allGood ? "SUCCESS" : "PARTIAL (some modules failed)");
    Serial.print("Total modules detected: ");
    Serial.println(manager.getAvailableCount());
  }
}

// ============================================================================
// CONTROL LOGIC: Example plant care automation
// ============================================================================

void updateControlLogic()
{
  ModuleManager &manager = ModuleManager::getInstance();

  // ========== IRRIGATION CONTROL ==========
  // Turn on pump if soil is dry
  if (manager.isModuleAvailable(MODULE_SOIL_MOISTURE) &&
      manager.isModuleAvailable(MODULE_PUMP))
  {

    Sensor *moistureSensor = manager.getSensor(MODULE_SOIL_MOISTURE);
    Actuator *pump = manager.getActuator(MODULE_PUMP);

    if (moistureSensor && pump)
    {
      SoilMoistureSensor *soil = (SoilMoistureSensor *)moistureSensor;

      if (DEBUG_ENABLED)
      {
        static unsigned long lastPumpDebug = 0;
        if (millis() - lastPumpDebug > 5000) // Debug every 5 seconds
        {
          Serial.print("[Pump Control] Soil Raw: ");
          Serial.print(soil->getRawValue());
          Serial.print(" | Dry Threshold: ");
          Serial.print(SOIL_MOISTURE_DRY_THRESHOLD);
          Serial.print(" | Wet Threshold: ");
          Serial.print(SOIL_MOISTURE_WET_THRESHOLD);
          Serial.print(" | isDry: ");
          Serial.print(soil->isDry());
          Serial.print(" | isWet: ");
          Serial.println(soil->isWet());
          lastPumpDebug = millis();
        }
      }

      // Check if soil is dry - only turn on if not already on
      if (soil->isDry() && !_pumpIsOn)
      {
        pump->on();
        _pumpIsOn = true;
        if (DEBUG_ENABLED)
        {
          Serial.println("[Pump] Turned ON (soil dry)");
        }
      }
      // Turn off pump if soil is wet - only turn off if not already off
      else if (soil->isWet() && _pumpIsOn)
      {
        pump->off();
        _pumpIsOn = false;
        if (DEBUG_ENABLED)
        {
          Serial.println("[Pump] Turned OFF (soil wet)");
        }
      }
    }
  }

  // ========== VENTILATION/FAN CONTROL ==========
  // Turn fan on if humidity is high OR temperature is high
  static unsigned long lastFanDebug = 0;
  bool dhtAvailable = manager.isModuleAvailable(MODULE_DHT);
  bool fanAvailable = manager.isModuleAvailable(MODULE_FAN);

  if (dhtAvailable && fanAvailable)
  {
    Sensor *dhtSensor = manager.getSensor(MODULE_DHT);
    Actuator *fanActuator = manager.getActuator(MODULE_FAN);

    if (dhtSensor && fanActuator)
    {
      DHTSensor *dht = (DHTSensor *)dhtSensor;
      VentilationFan *fan = (VentilationFan *)fanActuator;

      float temp = dht->getTemperature();
      float humidity = dht->getHumidity();
      bool tempHigh = dht->isTemperatureHigh();
      bool humidityHigh = dht->isHumidityHigh();
      bool shouldRunFan = (humidityHigh || tempHigh);

      // Debug output every 5 seconds
      if (DEBUG_ENABLED && millis() - lastFanDebug > 5000)
      {
        Serial.print("[Fan Control] Temp: ");
        Serial.print(temp);
        Serial.print("°C (high:");
        Serial.print(tempHigh ? "Y" : "N");
        Serial.print("), Humidity: ");
        Serial.print(humidity);
        Serial.print("% (high:");
        Serial.print(humidityHigh ? "Y" : "N");
        Serial.print("), shouldRun: ");
        Serial.print(shouldRunFan ? "Y" : "N");
        Serial.print(", _fanIsOn: ");
        Serial.println(_fanIsOn ? "Y" : "N");
        lastFanDebug = millis();
      }

      if (shouldRunFan && !_fanIsOn)
      {
        if (DEBUG_ENABLED)
        {
          Serial.print("[Main] *** Calling fan.on() - shouldRunFan=true, fanIsOn=false ***");
          Serial.print(" | Fan available: ");
          Serial.println(fan->isAvailable());
        }
        fan->on();
        _fanIsOn = true;
        if (DEBUG_ENABLED)
        {
          Serial.println("[Main] Fan: ON (high humidity/temperature)");
        }
      }
      else if (!shouldRunFan && _fanIsOn)
      {
        fan->off();
        _fanIsOn = false;
        if (DEBUG_ENABLED)
        {
          Serial.println("[Main] Fan: OFF (conditions normalized)");
        }
      }
    }
  }
  else if (DEBUG_ENABLED && millis() - lastFanDebug > 10000)
  {
    Serial.print("[Fan Debug] DHT available: ");
    Serial.print(dhtAvailable);
    Serial.print(", Fan available: ");
    Serial.println(fanAvailable);
    lastFanDebug = millis();
  }

  // ========== SUPPLEMENTAL LIGHTING CONTROL ==========
  // Turn on LED light if ambient light is insufficient
  if (manager.isModuleAvailable(MODULE_LIGHT_SENSOR) &&
      manager.isModuleAvailable(MODULE_LED_LIGHT))
  {

    Sensor *lightSensor = manager.getSensor(MODULE_LIGHT_SENSOR);
    Actuator *ledLight = manager.getActuator(MODULE_LED_LIGHT);

    if (lightSensor && ledLight)
    {
      // Support both BH1750 and PhotoresistorSensor
#if USE_BH1750_LIGHT_SENSOR
      BH1750Sensor *bh1750 = (BH1750Sensor *)lightSensor;
      if (bh1750->needsSupplementalLight())
      {
        ledLight->on();
        ((LEDGrowLight *)ledLight)->setBrightness(200); // 78% brightness
      }
      else
      {
        ledLight->off();
      }
#elif USE_PHOTORESISTOR_LIGHT_SENSOR
      PhotoresistorSensor *photoresistor = (PhotoresistorSensor *)lightSensor;
      if (photoresistor->needsSupplementalLight())
      {
        ledLight->on();
        ((LEDGrowLight *)ledLight)->setBrightness(200); // 78% brightness
      }
      else
      {
        ledLight->off();
      }
#endif
    }
  }

  // ========== CAMERA TIME-LAPSE ==========
  // Enable automatic image capture every 10 minutes
  if (manager.isModuleAvailable(MODULE_ESP32_CAM))
  {
    Module *camModule = manager.getModule(MODULE_ESP32_CAM);
    ESP32CAM *cam = (ESP32CAM *)camModule;

    // Set 10-minute interval (600000 ms) if not already set
    if (cam->getLastCaptureTime() == 0)
    {
      cam->setAutoCapInterval(600000);
    }
  }
}

// ============================================================================
// DEBUG: Print system status to Serial
// ============================================================================

void printSystemStatus()
{
  ModuleManager &manager = ModuleManager::getInstance();

  Serial.println("\n========== SYSTEM STATUS ==========");

  // Uptime
  unsigned long uptime = millis() / 1000;
  Serial.print("Uptime: ");
  Serial.print(uptime);
  Serial.println(" seconds");

  // Soil Moisture
  if (manager.isModuleAvailable(MODULE_SOIL_MOISTURE))
  {
    SoilMoistureSensor *moisture =
        (SoilMoistureSensor *)manager.getSensor(MODULE_SOIL_MOISTURE);
    if (moisture)
    {
      Serial.print("  Soil Moisture: ");
      Serial.print(moisture->getMoisturePercentage());
      Serial.print("% (Raw: ");
      Serial.print(moisture->getRawValue());
      Serial.println(")");
    }
  }

  // Temperature & Humidity
  if (manager.isModuleAvailable(MODULE_DHT))
  {
    DHTSensor *dht = (DHTSensor *)manager.getSensor(MODULE_DHT);
    if (dht)
    {
      Serial.print("  Temperature: ");
      Serial.print(dht->getTemperature());
      Serial.print("°C | Humidity: ");
      Serial.print(dht->getHumidity());
      Serial.println("% RH");
    }
  }

  // Light Level
  if (manager.isModuleAvailable(MODULE_LIGHT_SENSOR))
  {
    Sensor *light = manager.getSensor(MODULE_LIGHT_SENSOR);
    if (light)
    {
      Serial.print("  Light Level: ");
      Serial.print(light->getValue());
      Serial.println(" lux");
    }
  }

  // Pump Status
  if (manager.isModuleAvailable(MODULE_PUMP))
  {
    WaterPump *pump = (WaterPump *)manager.getActuator(MODULE_PUMP);
    if (pump)
    {
      Serial.print("  Pump: ");
      Serial.println(pump->isOn() ? "ON" : "OFF");
    }
  }

  // Ventilation Fan Status
  if (manager.isModuleAvailable(MODULE_FAN))
  {
    VentilationFan *fan = (VentilationFan *)manager.getActuator(MODULE_FAN);
    if (fan)
    {
      Serial.print("  Fan: ");
      Serial.println(fan->isOn() ? "ON" : "OFF");
    }
  }

  // LED Light Status
  if (manager.isModuleAvailable(MODULE_LED_LIGHT))
  {
    LEDGrowLight *led = (LEDGrowLight *)manager.getActuator(MODULE_LED_LIGHT);
    if (led)
    {
      Serial.print("  LED Light: ");
      Serial.print(led->isOn() ? "ON" : "OFF");
      Serial.print(" (Brightness: ");
      Serial.print((led->getBrightness() * 100) / 255);
      Serial.println("%)");
    }
  }

  // Camera Status
  if (manager.isModuleAvailable(MODULE_ESP32_CAM))
  {
    ESP32CAM *cam = (ESP32CAM *)manager.getModule(MODULE_ESP32_CAM);
    if (cam)
    {
      Serial.print("  Camera: ");
      Serial.print(cam->isReady() ? "READY" : "NOT READY");
      Serial.print(" | Images captured: ");
      Serial.println(cam->getImageCount());
    }
  }

  Serial.println("===================================\n");
}