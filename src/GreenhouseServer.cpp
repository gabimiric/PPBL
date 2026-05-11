#include "GreenhouseServer.h"
#include "secrets.h"
#include "config.h"
#include "Settings.h"
#include "ModuleManager.h"
#include "SoilMoistureSensor.h"
#include "DHTSensor.h"
#include "BH1750Sensor.h"
#include "PhotoresistorSensor.h"
#include "WaterPump.h"
#include "VentilationFan.h"
#include "LEDGrowLight.h"
#include "ESP32CAM.h"

#define CORS_HEADERS \
    "Access-Control-Allow-Origin: *\r\n" \
    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" \
    "Access-Control-Allow-Headers: Content-Type, X-Auth-Token\r\n"

// True if a non-empty API token is configured in secrets.h.
static inline bool authRequired()
{
    return API_TOKEN[0] != '\0';
}

// Cap on body size we are willing to read so a malformed client can't pin us.
static const size_t MAX_BODY_BYTES = 512;
static const unsigned long CLIENT_TIMEOUT_MS = 1500;

GreenhouseServer &GreenhouseServer::instance()
{
    static GreenhouseServer s;
    return s;
}

bool GreenhouseServer::begin()
{
    if (WiFi.status() == WL_NO_MODULE)
    {
        if (DEBUG_ENABLED)
            Serial.println("[Net] WiFi module not present");
        return false;
    }

    if (DEBUG_ENABLED)
    {
        Serial.print("[Net] Connecting to '");
        Serial.print(WIFI_SSID);
        Serial.print("'");
    }

    // Kick off the connection once and poll WiFi.status(). Calling
    // WiFi.begin() in a loop resets WiFiS3 state on every call, which can
    // break association on some firmware versions.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long connectStart = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - connectStart) < 20000)
    {
        if (DEBUG_ENABLED) Serial.print('.');
        delay(500);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        if (DEBUG_ENABLED)
            Serial.println("\n[Net] WiFi connect FAILED — running offline");
        _wifiReady = false;
        return false;
    }

    // WiFiS3 can report WL_CONNECTED before DHCP assigns an address.
    // Wait for a usable IP so we don't announce http://0.0.0.0.
    unsigned long dhcpStart = millis();
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && (millis() - dhcpStart) < 8000)
    {
        if (DEBUG_ENABLED) Serial.print('+');
        delay(250);
    }

    if (WiFi.localIP() == IPAddress(0, 0, 0, 0))
    {
        if (DEBUG_ENABLED)
            Serial.println("\n[Net] DHCP timed out — no IP assigned. Running offline.");
        _wifiReady = false;
        return false;
    }

    _server.begin();
    _wifiReady = true;

    if (DEBUG_ENABLED)
    {
        Serial.println();
        Serial.print("[Net] Connected. Open the dashboard and point it at: http://");
        Serial.println(WiFi.localIP());
    }
    return true;
}

IPAddress GreenhouseServer::getIP() const
{
    return WiFi.localIP();
}

void GreenhouseServer::handle()
{
    if (!_wifiReady) return;

    WiFiClient client = _server.available();
    if (!client) return;

    client.setTimeout(200);
    handleClient(client);
    delay(1);
    client.stop();
}

void GreenhouseServer::handleClient(WiFiClient &client)
{
    unsigned long start = millis();
    while (!client.available() && (millis() - start) < CLIENT_TIMEOUT_MS)
    {
        delay(1);
    }
    if (!client.available()) return;

    String requestLine = client.readStringUntil('\n');
    requestLine.replace("\r", "");
    requestLine.trim();
    if (requestLine.length() == 0) return;

    int sp1 = requestLine.indexOf(' ');
    int sp2 = requestLine.indexOf(' ', sp1 + 1);
    if (sp1 < 0 || sp2 < 0) return;

    String method = requestLine.substring(0, sp1);
    String url = requestLine.substring(sp1 + 1, sp2);

    // Drain headers, capture Content-Length and X-Auth-Token.
    long contentLength = 0;
    String authToken;
    while (client.connected())
    {
        String header = client.readStringUntil('\n');
        header.replace("\r", "");
        if (header.length() == 0) break;
        int colon = header.indexOf(':');
        if (colon > 0)
        {
            String name = header.substring(0, colon);
            String value = header.substring(colon + 1);
            name.trim(); value.trim();
            name.toLowerCase();
            if (name == "content-length") contentLength = value.toInt();
            else if (name == "x-auth-token") authToken = value;
        }
    }

    String body;
    if (contentLength > 0)
    {
        size_t want = (size_t)contentLength;
        if (want > MAX_BODY_BYTES) want = MAX_BODY_BYTES;
        body.reserve(want);
        unsigned long t0 = millis();
        while (body.length() < want && (millis() - t0) < CLIENT_TIMEOUT_MS)
        {
            while (client.available() && body.length() < want)
                body += (char)client.read();
        }
    }

    String path = url, query;
    int q = url.indexOf('?');
    if (q >= 0)
    {
        path = url.substring(0, q);
        query = url.substring(q + 1);
    }

    if (DEBUG_ENABLED)
    {
        Serial.print("[HTTP] ");
        Serial.print(method);
        Serial.print(' ');
        Serial.println(url);
    }

    if (method == "OPTIONS")
    {
        sendCorsPreflight(client);
        return;
    }

    // Auth gate: /api/* requires a matching X-Auth-Token when a token is set
    // in secrets.h. The static info page at "/" stays public so a browser
    // visit confirms the device is up before tokens are configured.
    if (authRequired() && path.startsWith("/api/") && authToken != API_TOKEN)
    {
        sendError(client, 401, "unauthorized");
        return;
    }

    if (method == "GET")
        routeGet(client, path);
    else if (method == "POST")
        routePost(client, path, query, body);
    else
        sendError(client, 405, "method not allowed");
}

void GreenhouseServer::routeGet(WiFiClient &client, const String &path)
{
    if (path == "/api/state")
        serveState(client);
    else if (path == "/" || path == "/index.html")
        serveInfoPage(client);
    else
        sendError(client, 404, "not found");
}

void GreenhouseServer::routePost(WiFiClient &client, const String &path, const String &query, const String &body)
{
    (void)body; // commands use query params (avoids CORS preflight)

    if (path == "/api/actuator")     doActuator(client, query);
    else if (path == "/api/mode")    doMode(client, query);
    else if (path == "/api/threshold") doThreshold(client, query);
    else if (path == "/api/snapshot")  doSnapshot(client);
    else sendError(client, 404, "not found");
}

// ----------------------------------------------------------------------------
// Endpoint: GET /api/state
// ----------------------------------------------------------------------------
void GreenhouseServer::serveState(WiFiClient &client)
{
    ModuleManager &mm = ModuleManager::getInstance();
    Settings &cfg = Settings::instance();

    auto avail = [&](ModuleType t) { return mm.isModuleAvailable(t); };

    float soilPct = 0, temp = 0, hum = 0;
    uint16_t soilRaw = 0;
    float lux = 0;

    if (avail(MODULE_SOIL_MOISTURE))
    {
        SoilMoistureSensor *s = (SoilMoistureSensor *)mm.getSensor(MODULE_SOIL_MOISTURE);
        if (s) { soilPct = s->getMoisturePercentage(); soilRaw = s->getRawValue(); }
    }
    if (avail(MODULE_DHT))
    {
        DHTSensor *d = (DHTSensor *)mm.getSensor(MODULE_DHT);
        if (d) { temp = d->getTemperature(); hum = d->getHumidity(); }
    }
    if (avail(MODULE_LIGHT_SENSOR))
    {
        Sensor *ls = mm.getSensor(MODULE_LIGHT_SENSOR);
        if (ls) lux = ls->getValue();
    }

    bool pumpOn = false, fanOn = false, ledOn = false;
    uint8_t ledBrightness = 0;
    if (avail(MODULE_PUMP))
    {
        WaterPump *p = (WaterPump *)mm.getActuator(MODULE_PUMP);
        if (p) pumpOn = p->isOn();
    }
    if (avail(MODULE_FAN))
    {
        VentilationFan *f = (VentilationFan *)mm.getActuator(MODULE_FAN);
        if (f) fanOn = f->isOn();
    }
    if (avail(MODULE_LED_LIGHT))
    {
        LEDGrowLight *l = (LEDGrowLight *)mm.getActuator(MODULE_LED_LIGHT);
        if (l) { ledOn = l->isOn(); ledBrightness = l->getBrightness(); }
    }

    uint32_t imageCount = 0;
    bool camReady = false;
    if (avail(MODULE_ESP32_CAM))
    {
        ESP32CAM *c = (ESP32CAM *)mm.getModule(MODULE_ESP32_CAM);
        if (c) { imageCount = c->getImageCount(); camReady = c->isReady(); }
    }

    String json;
    json.reserve(640);
    json += '{';
    json += "\"ok\":true";
    json += ",\"uptime\":" + String(millis() / 1000UL);
    json += ",\"mode\":\"" + String(cfg.mode == MODE_AUTO ? "auto" : "manual") + "\"";
    json += ",\"soil\":" + String(soilPct, 1);
    json += ",\"soil_raw\":" + String(soilRaw);
    json += ",\"temp\":" + String(temp, 1);
    json += ",\"hum\":" + String(hum, 1);
    json += ",\"lux\":" + String(lux, 1);
    json += ",\"pump\":" + String(pumpOn ? "true" : "false");
    json += ",\"fan\":"  + String(fanOn  ? "true" : "false");
    json += ",\"led\":"  + String(ledOn  ? "true" : "false");
    json += ",\"led_brightness\":" + String(ledBrightness);
    json += ",\"thresholds\":{";
    json += "\"soilDry\":" + String(cfg.soilDryThreshold);
    json += ",\"soilWet\":" + String(cfg.soilWetThreshold);
    json += ",\"hum\":"  + String(cfg.humidityHigh);
    json += ",\"temp\":" + String(cfg.tempHigh);
    json += ",\"lux\":"  + String(cfg.lightLow);
    json += '}';
    json += ",\"auth\":" + String(authRequired() ? "true" : "false");
    json += ",\"available\":{";
    json += "\"soil\":"  + String(avail(MODULE_SOIL_MOISTURE) ? "true" : "false");
    json += ",\"dht\":"  + String(avail(MODULE_DHT) ? "true" : "false");
    json += ",\"light\":"+ String(avail(MODULE_LIGHT_SENSOR) ? "true" : "false");
    json += ",\"pump\":" + String(avail(MODULE_PUMP) ? "true" : "false");
    json += ",\"fan\":"  + String(avail(MODULE_FAN) ? "true" : "false");
    json += ",\"led\":"  + String(avail(MODULE_LED_LIGHT) ? "true" : "false");
    json += ",\"cam\":"  + String(camReady ? "true" : "false");
    json += '}';
    json += ",\"images\":" + String(imageCount);
    json += '}';

    sendJson(client, 200, json);
}

// ----------------------------------------------------------------------------
// Endpoint: POST /api/actuator?id=pump|fan|led&state=on|off
// ----------------------------------------------------------------------------
void GreenhouseServer::doActuator(WiFiClient &client, const String &query)
{
    String id = queryParam(query, "id");
    String state = queryParam(query, "state");
    if (id.length() == 0 || state.length() == 0)
    {
        sendError(client, 400, "missing id or state");
        return;
    }

    bool on = (state == "on" || state == "1" || state == "true");
    Settings &cfg = Settings::instance();
    ModuleManager &mm = ModuleManager::getInstance();

    ModuleType type;
    bool *manualFlag = nullptr;
    if (id == "pump") { type = MODULE_PUMP; manualFlag = &cfg.manualPump; }
    else if (id == "fan")  { type = MODULE_FAN;  manualFlag = &cfg.manualFan; }
    else if (id == "led")  { type = MODULE_LED_LIGHT; manualFlag = &cfg.manualLed; }
    else { sendError(client, 400, "unknown id"); return; }

    if (!mm.isModuleAvailable(type))
    {
        sendError(client, 503, "actuator not available");
        return;
    }
    Actuator *act = mm.getActuator(type);
    if (!act) { sendError(client, 503, "actuator missing"); return; }

    *manualFlag = on;
    if (on) act->on(); else act->off();

    sendOk(client, String("\"") + id + "\":\"" + (on ? "on" : "off") + "\"");
}

// ----------------------------------------------------------------------------
// Endpoint: POST /api/mode?value=auto|manual
// ----------------------------------------------------------------------------
void GreenhouseServer::doMode(WiFiClient &client, const String &query)
{
    String v = queryParam(query, "value");
    Settings &cfg = Settings::instance();
    ControlMode previous = cfg.mode;
    if (v == "auto")        cfg.mode = MODE_AUTO;
    else if (v == "manual") cfg.mode = MODE_MANUAL;
    else { sendError(client, 400, "value must be auto|manual"); return; }

    // When entering manual mode, seed the manual flags with current actuator state
    // so the dashboard's toggles reflect reality.
    if (cfg.mode == MODE_MANUAL)
    {
        ModuleManager &mm = ModuleManager::getInstance();
        Actuator *p = mm.getActuator(MODULE_PUMP);
        Actuator *f = mm.getActuator(MODULE_FAN);
        Actuator *l = mm.getActuator(MODULE_LED_LIGHT);
        if (p) cfg.manualPump = p->isOn();
        if (f) cfg.manualFan  = f->isOn();
        if (l) cfg.manualLed  = l->isOn();
    }

    if (cfg.mode != previous) cfg.markDirty();

    sendOk(client, String("\"mode\":\"") + v + "\"");
}

// ----------------------------------------------------------------------------
// Endpoint: POST /api/threshold?key=soilDry|soilWet|hum|temp|lux&value=N
// ----------------------------------------------------------------------------
void GreenhouseServer::doThreshold(WiFiClient &client, const String &query)
{
    String key = queryParam(query, "key");
    String valueStr = queryParam(query, "value");
    if (key.length() == 0 || valueStr.length() == 0)
    {
        sendError(client, 400, "missing key or value");
        return;
    }

    long v = valueStr.toInt();
    Settings &cfg = Settings::instance();
    bool changed = true;
    if (key == "soilDry")      cfg.soilDryThreshold = (uint16_t)constrain(v, 0, 1023);
    else if (key == "soilWet") cfg.soilWetThreshold = (uint16_t)constrain(v, 0, 1023);
    else if (key == "hum")     cfg.humidityHigh = (uint8_t)constrain(v, 0, 100);
    else if (key == "temp")    cfg.tempHigh = (uint8_t)constrain(v, 0, 60);
    else if (key == "lux")     cfg.lightLow = (uint16_t)constrain(v, 0, 65535);
    else { changed = false; sendError(client, 400, "unknown key"); return; }

    if (changed) cfg.markDirty();

    sendOk(client, String("\"") + key + "\":" + valueStr);
}

// ----------------------------------------------------------------------------
// Endpoint: POST /api/snapshot
// ----------------------------------------------------------------------------
void GreenhouseServer::doSnapshot(WiFiClient &client)
{
    ModuleManager &mm = ModuleManager::getInstance();
    if (!mm.isModuleAvailable(MODULE_ESP32_CAM))
    {
        sendError(client, 503, "camera not available");
        return;
    }
    ESP32CAM *cam = (ESP32CAM *)mm.getModule(MODULE_ESP32_CAM);
    bool ok = cam ? cam->captureImage() : false;
    if (ok) sendOk(client, "\"captured\":true");
    else sendError(client, 500, "capture failed");
}

// ----------------------------------------------------------------------------
// Tiny browser-facing info page (so visiting http://<ip>/ shows something).
// ----------------------------------------------------------------------------
void GreenhouseServer::serveInfoPage(WiFiClient &client)
{
    String body =
        "<!doctype html><meta charset=utf-8>"
        "<title>GreenPod node</title>"
        "<style>body{font-family:system-ui;background:#0a0e0c;color:#e8efe9;padding:32px;line-height:1.5}"
        "code{background:#161d19;padding:2px 6px;border-radius:4px;color:#4ade80}"
        "a{color:#4ade80}</style>"
        "<h1>🌱 GreenPod node online</h1>"
        "<p>This is the Arduino REST endpoint. Open the dashboard (<code>index.html</code>) and "
        "set the host to <code>http://";
    body += WiFi.localIP().toString();
    body += "</code>.</p><ul>"
        "<li><code>GET /api/state</code></li>"
        "<li><code>POST /api/actuator?id=pump&amp;state=on</code></li>"
        "<li><code>POST /api/mode?value=manual</code></li>"
        "<li><code>POST /api/threshold?key=temp&amp;value=27</code></li>"
        "<li><code>POST /api/snapshot</code></li></ul>";

    String headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: text/html; charset=utf-8\r\n";
    headers += CORS_HEADERS;
    headers += "Content-Length: " + String(body.length()) + "\r\n";
    headers += "Connection: close\r\n\r\n";
    client.print(headers);
    client.print(body);
}

// ----------------------------------------------------------------------------
// Response helpers
// ----------------------------------------------------------------------------
void GreenhouseServer::sendJson(WiFiClient &client, int status, const String &json)
{
    String headers = "HTTP/1.1 " + String(status) + " OK\r\n";
    headers += "Content-Type: application/json\r\n";
    headers += CORS_HEADERS;
    headers += "Content-Length: " + String(json.length()) + "\r\n";
    headers += "Connection: close\r\n\r\n";
    client.print(headers);
    client.print(json);
}

void GreenhouseServer::sendOk(WiFiClient &client, const String &message)
{
    sendJson(client, 200, String("{\"ok\":true,") + message + "}");
}

void GreenhouseServer::sendError(WiFiClient &client, int status, const String &message)
{
    String body = String("{\"ok\":false,\"error\":\"") + escapeJson(message) + "\"}";
    String headers = "HTTP/1.1 " + String(status) + " Error\r\n";
    headers += "Content-Type: application/json\r\n";
    headers += CORS_HEADERS;
    headers += "Content-Length: " + String(body.length()) + "\r\n";
    headers += "Connection: close\r\n\r\n";
    client.print(headers);
    client.print(body);
}

void GreenhouseServer::sendCorsPreflight(WiFiClient &client)
{
    String headers = "HTTP/1.1 204 No Content\r\n";
    headers += CORS_HEADERS;
    headers += "Access-Control-Max-Age: 3600\r\n";
    headers += "Content-Length: 0\r\n";
    headers += "Connection: close\r\n\r\n";
    client.print(headers);
}

// ----------------------------------------------------------------------------
// Utilities
// ----------------------------------------------------------------------------
String GreenhouseServer::urlDecode(const String &s)
{
    String out;
    out.reserve(s.length());
    for (size_t i = 0; i < s.length(); i++)
    {
        char c = s[i];
        if (c == '+') { out += ' '; }
        else if (c == '%' && i + 2 < s.length())
        {
            char hi = s[i + 1], lo = s[i + 2];
            auto hex = [](char ch) -> int {
                if (ch >= '0' && ch <= '9') return ch - '0';
                if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                return -1;
            };
            int h = hex(hi), l = hex(lo);
            if (h >= 0 && l >= 0) { out += (char)((h << 4) | l); i += 2; }
            else out += c;
        }
        else out += c;
    }
    return out;
}

String GreenhouseServer::queryParam(const String &query, const String &key)
{
    int from = 0;
    while (from < (int)query.length())
    {
        int amp = query.indexOf('&', from);
        int end = amp < 0 ? query.length() : amp;
        int eq = query.indexOf('=', from);
        if (eq > 0 && eq < end)
        {
            String k = query.substring(from, eq);
            if (k == key)
            {
                String v = query.substring(eq + 1, end);
                return urlDecode(v);
            }
        }
        if (amp < 0) break;
        from = amp + 1;
    }
    return "";
}

String GreenhouseServer::escapeJson(const String &s)
{
    String out;
    out.reserve(s.length() + 4);
    for (size_t i = 0; i < s.length(); i++)
    {
        char c = s[i];
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\r') { out += "\\r"; }
        else out += c;
    }
    return out;
}
