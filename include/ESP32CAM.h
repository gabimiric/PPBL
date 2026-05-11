#ifndef ESP32_CAM_H
#define ESP32_CAM_H

#include "Module.h"
#include "config.h"

// ============================================================================
// ESP32-CAM CAMERA MODULE DRIVER
// ============================================================================

class ESP32CAM : public Module
{
public:
    ESP32CAM(uint8_t rxPin = ESP32_CAM_RX_PIN, uint8_t txPin = ESP32_CAM_TX_PIN);
    virtual ~ESP32CAM() {}

    bool init() override;
    bool isAvailable() override;
    void update() override;

    const char *getName() const override { return "ESP32-CAM"; }
    uint8_t getModuleType() const override { return MODULE_ESP32_CAM; }

    /**
     * Trigger image capture.
     * Image is stored on microSD card with timestamp.
     */
    bool captureImage();

    /**
     * Set interval for automatic time-lapse captures (ms).
     * Set to 0 to disable automatic capture.
     */
    void setAutoCapInterval(unsigned long intervalMs);

    /**
     * Check if ESP32-CAM is ready and responding.
     */
    bool isReady() const { return _ready; }

    /**
     * Get last capture timestamp.
     */
    unsigned long getLastCaptureTime() const { return _lastCaptureTime; }

    /**
     * Get number of images captured so far.
     */
    uint32_t getImageCount() const { return _imageCount; }

private:
    uint8_t _rxPin;
    uint8_t _txPin;
    bool _ready = false;
    unsigned long _lastCaptureTime = 0;
    unsigned long _lastUpdateTime = 0;
    unsigned long _autoCapInterval = 0; // 0 = disabled
    uint32_t _imageCount = 0;

    /**
     * Send AT command to ESP32-CAM and get response.
     */
    bool sendATCommand(const char *cmd, unsigned long timeoutMs = 1000);

    /**
     * Initialize serial communication with ESP32-CAM.
     */
    bool initializeSerial();

    /**
     * Handle automatic time-lapse capture.
     */
    void handleAutoCap();
};

#endif // ESP32_CAM_H
