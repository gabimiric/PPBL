#include <Arduino.h>
#include "config.h"
#include "Settings.h"
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
#include "GreenhouseServer.h"

// ============================================================================
// SYSTEM-LEVEL CONTROL LOGIC
// ============================================================================

// Forward declarations
void initializeSystem();
void updateSensors();
void updateControlLogic();
void printSystemStatus();

// Note: actuator on()/off() are idempotent and self-debounce their internal
// state, so main.cpp no longer caches _pumpIsOn / _fanIsOn — we just call the
// methods directly and let the actuator decide whether anything changed.

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

  // Restore persisted thresholds + mode (must happen before module init so
  // module-side defaults can use the loaded values if they care).
  Settings::instance().load();

  // Initialize and detect all modules
  initializeSystem();

  // One-time camera time-lapse setup (10-minute interval). The control loop
  // previously did this lazily, which made the intent harder to follow.
  ModuleManager &manager = ModuleManager::getInstance();
  if (manager.isModuleAvailable(MODULE_ESP32_CAM))
  {
    ESP32CAM *cam = (ESP32CAM *)manager.getModule(MODULE_ESP32_CAM);
    if (cam) cam->setAutoCapInterval(600000);
  }

  // Start WiFi + REST API for the dashboard
  GreenhouseServer::instance().begin();

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
  if (display.isAvailable())
  {
    display.update();
  }

  // Serve any pending HTTP requests from the dashboard
  GreenhouseServer::instance().handle();

  // Persist any pending settings changes (debounced, so rapid slider drags
  // coalesce into one flash write).
  Settings::instance().flushIfDue();

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

  // Initialize all configured modules
  // This will auto-detect which modules are actually connected
  bool allGood = manager.initializeAll();

  // Initialize the OLED display last so the rest of the system still boots
  // when no display is attached.
  if (!display.init() && DEBUG_ENABLED)
  {
    Serial.println("[Main] Warning: Display initialization failed");
  }

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
  Settings &cfg = Settings::instance();
  unsigned long now = millis();

  auto overrideActive = [&](unsigned long untilMs) {
    return untilMs != 0 && (long)(now - untilMs) < 0;
  };

  // ========== MANUAL MODE ==========
  // In manual mode the dashboard owns the actuators; skip automation and
  // mirror the latest manual commands. on()/off() are idempotent so the
  // "are we already in the right state?" check is just an optimisation to
  // avoid spamming logs.
  if (cfg.mode == MODE_MANUAL)
  {
    Actuator *p = manager.getActuator(MODULE_PUMP);
    Actuator *f = manager.getActuator(MODULE_FAN);
    Actuator *l = manager.getActuator(MODULE_LED_LIGHT);
    if (p) { cfg.manualPump ? p->on() : p->off(); }
    if (f) { cfg.manualFan  ? f->on() : f->off(); }
    if (l) { cfg.manualLed  ? l->on() : l->off(); }
    return;
  }

  // ========== IRRIGATION CONTROL ==========
  // Pump runs only while the soil is dry. The safety timeout in WaterPump
  // limits each pulse so the sensor can settle before the next decision.
  if (manager.isModuleAvailable(MODULE_SOIL_MOISTURE) &&
      manager.isModuleAvailable(MODULE_PUMP))
  {
    SoilMoistureSensor *soil = (SoilMoistureSensor *)manager.getSensor(MODULE_SOIL_MOISTURE);
    WaterPump *pump = (WaterPump *)manager.getActuator(MODULE_PUMP);

    if (soil && pump)
    {
      if (overrideActive(cfg.manualPumpOverrideUntil))
      {
        cfg.manualPump ? pump->on() : pump->off();
      }
      else
      {
        if (soil->isDry())
        {
          if (!pump->isOn() && !pump->isCooldownActive())
          {
            pump->on();
          }
        }
        else
        {
          if (pump->isOn())
          {
            pump->off();
            pump->startCooldown();
          }
        }
      }
    }
  }

  // ========== VENTILATION/FAN CONTROL ==========
  if (manager.isModuleAvailable(MODULE_DHT) && manager.isModuleAvailable(MODULE_FAN))
  {
    DHTSensor *dht = (DHTSensor *)manager.getSensor(MODULE_DHT);
    VentilationFan *fan = (VentilationFan *)manager.getActuator(MODULE_FAN);

    if (dht && fan)
    {
      if (overrideActive(cfg.manualFanOverrideUntil))
      {
        cfg.manualFan ? fan->on() : fan->off();
      }
      else
      {
      bool tempHigh = dht->getTemperature() > (float)cfg.tempHigh;
      bool humidityHigh = dht->getHumidity() > (float)cfg.humidityHigh;
      bool shouldRunFan = (humidityHigh || tempHigh);

      if (shouldRunFan) fan->on();
      else fan->off();
      }
    }
  }

  // ========== SUPPLEMENTAL LIGHTING CONTROL ==========
  if (manager.isModuleAvailable(MODULE_LIGHT_SENSOR) &&
      manager.isModuleAvailable(MODULE_LED_LIGHT))
  {
    Sensor *lightSensor = manager.getSensor(MODULE_LIGHT_SENSOR);
    LEDGrowLight *ledLight = (LEDGrowLight *)manager.getActuator(MODULE_LED_LIGHT);

    if (lightSensor && ledLight)
    {
      if (overrideActive(cfg.manualLedOverrideUntil))
      {
        cfg.manualLed ? ledLight->on() : ledLight->off();
      }
      else
      {
      float reading = lightSensor->getValue();
      bool needsLight = reading < (float)cfg.lightLow;
      if (needsLight) { ledLight->setBrightness(200); ledLight->on(); }
      else { ledLight->off(); }
      }
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