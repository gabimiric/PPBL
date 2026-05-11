#include "DisplayManager.h"
#include "ModuleManager.h"
#include "DHTSensor.h"
#include "SoilMoistureSensor.h"
#include "PhotoresistorSensor.h"
#include "WaterPump.h"
#include "VentilationFan.h"
#include "LEDGrowLight.h"

DisplayManager &DisplayManager::getInstance()
{
    static DisplayManager instance;
    return instance;
}

bool DisplayManager::init()
{
    // Initialize the display with I2C address
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR))
    {
        if (DEBUG_ENABLED)
        {
            Serial.println("[DisplayManager] SSD1306 allocation failed");
        }
        _available = false;
        return false;
    }

    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);
    _display.println("PPBL System");
    _display.println("Initializing...");
    _display.display();

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.println("[DisplayManager] SSD1306 OLED initialized");
    }

    return true;
}

void DisplayManager::update()
{
    unsigned long now = millis();

    // Update display every 100ms
    if (now - _lastUpdateTime < 100)
    {
        return;
    }

    drawStatusTable();
    _lastUpdateTime = now;
}

void DisplayManager::togglePage()
{
    _currentPage = (_currentPage + 1) % 2; // Toggle between 0 and 1

    if (DEBUG_ENABLED)
    {
        Serial.print("[DisplayManager] Switched to page ");
        Serial.println(_currentPage);
    }
}

void DisplayManager::clear()
{
    _display.clearDisplay();
}

void DisplayManager::print(int16_t x, int16_t y, const char *text)
{
    _display.setCursor(x, y);
    _display.println(text);
}

float DisplayManager::getSensorValue(ModuleType type, float defaultValue)
{
    ModuleManager &manager = ModuleManager::getInstance();
    Sensor *sensor = manager.getSensor(type);

    if (sensor && sensor->isAvailable())
    {
        return sensor->getValue();
    }

    return defaultValue;
}

void DisplayManager::drawStatusTable()
{
    ModuleManager &manager = ModuleManager::getInstance();

    _display.clearDisplay();
    _display.setTextSize(1); // Size 1 = 6x8 pixels per char
    _display.setTextColor(SSD1306_WHITE);

    // Build all 8 status lines
    char statusLines[8][48];

    // Line 0: Header
    snprintf(statusLines[0], sizeof(statusLines[0]), "=== SYSTEM STATUS ===");

    // Line 1: Uptime
    unsigned long uptime = millis() / 1000;
    snprintf(statusLines[1], sizeof(statusLines[1]), "Uptime: %lu sec", uptime);

    // Line 2: Soil Moisture
    Sensor *soilSensor = manager.getSensor(MODULE_SOIL_MOISTURE);
    if (soilSensor && soilSensor->isAvailable())
    {
        float soilPercent = dynamic_cast<SoilMoistureSensor *>(soilSensor)->getMoisturePercentage();
        uint16_t soilRaw = dynamic_cast<SoilMoistureSensor *>(soilSensor)->getRawValue();
        snprintf(statusLines[2], sizeof(statusLines[2]), "Soil:%.1f%% Raw:%d", soilPercent, soilRaw);
    }
    else
    {
        snprintf(statusLines[2], sizeof(statusLines[2]), "Soil: N/A");
    }

    // Line 3: Temperature & Humidity
    Sensor *dhtSensor = manager.getSensor(MODULE_DHT);
    if (dhtSensor && dhtSensor->isAvailable())
    {
        DHTSensor *dht = dynamic_cast<DHTSensor *>(dhtSensor);
        snprintf(statusLines[3], sizeof(statusLines[3]), "T:%.1fC H:%.0f%%", dht->getTemperature(), dht->getHumidity());
    }
    else
    {
        snprintf(statusLines[3], sizeof(statusLines[3]), "T:N/A H:N/A");
    }

    // Line 4: Light Level
    Sensor *lightSensor = manager.getSensor(MODULE_LIGHT_SENSOR);
    if (lightSensor && lightSensor->isAvailable())
    {
        snprintf(statusLines[4], sizeof(statusLines[4]), "Light:%d raw", (int)lightSensor->getValue());
    }
    else
    {
        snprintf(statusLines[4], sizeof(statusLines[4]), "Light:N/A");
    }

    // Line 5: Pump Status
    WaterPump *pump = dynamic_cast<WaterPump *>(manager.getModule(MODULE_PUMP));
    const char *pumpStatus = (pump && pump->isAvailable() && pump->isOn()) ? "ON" : "OFF";
    snprintf(statusLines[5], sizeof(statusLines[5]), "Pump:%s", pumpStatus);

    // Line 6: Ventilation Fan
    VentilationFan *fan = dynamic_cast<VentilationFan *>(manager.getModule(MODULE_FAN));
    const char *fanStatus = (fan && fan->isAvailable() && fan->isOn()) ? "ON" : "OFF";
    snprintf(statusLines[6], sizeof(statusLines[6]), "Fan:%s", fanStatus);

    // Line 7: LED Light Status
    LEDGrowLight *led = dynamic_cast<LEDGrowLight *>(manager.getModule(MODULE_LED_LIGHT));
    if (led && led->isAvailable())
    {
        const char *ledStatus = led->isOn() ? "ON" : "OFF";
        uint8_t brightnessPercent = (led->getBrightness() * 100) / 255;
        snprintf(statusLines[7], sizeof(statusLines[7]), "LED:%s %d%%", ledStatus, brightnessPercent);
    }
    else
    {
        snprintf(statusLines[7], sizeof(statusLines[7]), "LED:N/A");
    }

    // Display the appropriate page (4 lines at a time)
    int startLine = _currentPage * 4;
    int y = 0;
    const int lineHeight = 8;

    for (int i = 0; i < 4; i++)
    {
        _display.setCursor(0, y);
        _display.println(statusLines[startLine + i]);
        y += lineHeight;
    }

    // Add page indicator at bottom
    _display.setCursor(120, 56);
    _display.print(_currentPage ? "P2" : "P1");

    _display.display();
}
