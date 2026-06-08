#ifndef GREENHOUSE_SERVER_H
#define GREENHOUSE_SERVER_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <IPAddress.h>

// Lightweight HTTP REST server that exposes the greenhouse state and accepts
// control commands from the web dashboard.
//
// Endpoints (all responses include CORS headers):
//   GET  /                  -> tiny HTML info page (handy from a browser)
//   GET  /api/state         -> JSON snapshot of sensors + actuators + config
//   POST /api/actuator?id=pump|fan|led&state=on|off
//   POST /api/mode?value=auto|manual
//   POST /api/threshold?key=soilDry|soilWet|hum|temp|lux&value=<number>
//   POST /api/snapshot      -> trigger ESP32-CAM capture
//   OPTIONS *               -> CORS preflight

class GreenhouseServer
{
public:
    static GreenhouseServer &instance();

    // Connect to WiFi (using credentials in secrets.h) and start the HTTP
    // server on port 80. Returns true on success.
    bool begin();

    // Handle a single pending client. Call from loop().
    void handle();

    bool isConnected() const { return _wifiReady; }
    IPAddress getIP() const;

private:
    GreenhouseServer() : _server(80) {}
    GreenhouseServer(const GreenhouseServer &) = delete;
    GreenhouseServer &operator=(const GreenhouseServer &) = delete;

    WiFiServer _server;
    bool _wifiReady = false;

    void handleClient(WiFiClient &client);

    void routeGet(WiFiClient &client, const String &path);
    void routePost(WiFiClient &client, const String &path, const String &query, const String &body);

    // Endpoint implementations
    void serveState(WiFiClient &client);
    void serveInfoPage(WiFiClient &client);
    void doActuator(WiFiClient &client, const String &query);
    void doMode(WiFiClient &client, const String &query);
    void doThreshold(WiFiClient &client, const String &query);
    void doSnapshot(WiFiClient &client);

    // Response helpers
    void sendJson(WiFiClient &client, int status, const String &json);
    void sendOk(WiFiClient &client, const String &message);
    void sendError(WiFiClient &client, int status, const String &message);
    void sendCorsPreflight(WiFiClient &client);

    // Utilities
    static String urlDecode(const String &s);
    static String queryParam(const String &query, const String &key);
    static String escapeJson(const String &s);
};

#endif // GREENHOUSE_SERVER_H
