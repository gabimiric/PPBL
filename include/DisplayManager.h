#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "Module.h"

// ============================================================================
// OLED DISPLAY MANAGER - 64x128 SSD1306 I2C DISPLAY
// ============================================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDR 0x3C

class DisplayManager
{
public:
    /**
     * Singleton instance getter.
     */
    static DisplayManager &getInstance();

    /**
     * Initialize the OLED display.
     * Returns true if display initialized successfully.
     */
    bool init();

    /**
     * Update the display with current system status.
     * Call this periodically (every 100ms).
     */
    void update();

    /**
     * Toggle between page 0 (lines 0-3) and page 1 (lines 4-7).
     */
    void togglePage();

    /**
     * Check if display is available.
     */
    bool isAvailable() const { return _available; }

    /**
     * Clear the display.
     */
    void clear();

    /**
     * Print text at specified position.
     */
    void print(int16_t x, int16_t y, const char *text);

private:
    // Singleton pattern
    DisplayManager() = default;
    DisplayManager(const DisplayManager &) = delete;
    DisplayManager &operator=(const DisplayManager &) = delete;

    Adafruit_SSD1306 _display;
    bool _available = false;
    unsigned long _lastUpdateTime = 0;
    uint8_t _currentPage = 0;  // 0 = lines 0-3, 1 = lines 4-7

    /**
     * Draw the status table on the display.
     */
    void drawStatusTable();

    /**
     * Get a sensor value safely (returns 0 if sensor not available).
     */
    float getSensorValue(ModuleType type, float defaultValue = 0.0f);
};

#endif // DISPLAY_MANAGER_H
