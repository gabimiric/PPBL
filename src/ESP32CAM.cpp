#include "ESP32CAM.h"
#include <SoftwareSerial.h>

// Software serial for ESP32-CAM communication
SoftwareSerial *_camSerial = nullptr;

ESP32CAM::ESP32CAM(uint8_t rxPin, uint8_t txPin)
    : _rxPin(rxPin), _txPin(txPin) {}

bool ESP32CAM::init()
{
    // Initialize software serial for ESP32-CAM communication
    if (!initializeSerial())
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.println("[ESP32-CAM] Failed to initialize serial communication");
        }
        return false;
    }

    delay(2000); // Give ESP32-CAM time to boot

    // Try to communicate with ESP32-CAM
    if (!isReady())
    {
        _available = false;
        if (DEBUG_ENABLED)
        {
            Serial.println("[ESP32-CAM] Module not responding");
        }
        return false;
    }

    _available = true;

    if (DEBUG_ENABLED)
    {
        Serial.println("[ESP32-CAM] Initialized successfully");
    }

    return true;
}

bool ESP32CAM::isAvailable()
{
    // Return the persistent availability flag set during init
    return _available;
}

void ESP32CAM::update()
{
    // Handle automatic time-lapse capture
    if (_autoCapInterval > 0)
    {
        handleAutoCap();
    }
}

bool ESP32CAM::captureImage()
{
    if (!_ready)
        return false;

    // Send capture command (implementation depends on ESP32-CAM firmware)
    // This is a simple example - adjust based on actual ESP32-CAM AT command set
    bool success = sendATCommand("AT+CAM=CAPTURE", 5000);

    if (success)
    {
        _lastCaptureTime = millis();
        _imageCount++;

        if (DEBUG_ENABLED)
        {
            Serial.print("[ESP32-CAM] Image captured. Total: ");
            Serial.println(_imageCount);
        }
    }

    return success;
}

void ESP32CAM::setAutoCapInterval(unsigned long intervalMs)
{
    _autoCapInterval = intervalMs;

    if (DEBUG_ENABLED)
    {
        if (intervalMs == 0)
        {
            Serial.println("[ESP32-CAM] Auto-capture disabled");
        }
        else
        {
            Serial.print("[ESP32-CAM] Auto-capture interval set to ");
            Serial.print(intervalMs);
            Serial.println(" ms");
        }
    }
}

bool ESP32CAM::sendATCommand(const char *cmd, unsigned long timeoutMs)
{
    if (!_camSerial)
        return false;

    // Clear any pending data
    while (_camSerial->available())
    {
        _camSerial->read();
    }

    // Send command
    _camSerial->println(cmd);

    // Wait for OK response
    unsigned long startTime = millis();
    String response = "";

    while (millis() - startTime < timeoutMs)
    {
        if (_camSerial->available())
        {
            char c = _camSerial->read();
            response += c;

            if (response.indexOf("OK") != -1)
            {
                return true;
            }
            if (response.indexOf("ERROR") != -1)
            {
                return false;
            }
        }
    }

    return false;
}

bool ESP32CAM::initializeSerial()
{
    // Create software serial instance
    _camSerial = new SoftwareSerial(_rxPin, _txPin);
    _camSerial->begin(115200);

    return _camSerial != nullptr;
}

void ESP32CAM::handleAutoCap()
{
    static unsigned long lastAutoCapTime = 0;

    if (millis() - lastAutoCapTime >= _autoCapInterval)
    {
        captureImage();
        lastAutoCapTime = millis();
    }
}
