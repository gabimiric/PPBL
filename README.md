# PPBL — Plant Care System

Autonomous smart-greenhouse firmware for the **Arduino UNO R4 WiFi** with an
optional ESP32-CAM for image capture and a single-file web dashboard for
monitoring/control.

## Architecture

```text
Module (Base)
├── Sensor
│   ├── SoilMoistureSensor      analog (A0)
│   ├── DHTSensor               DHT11/22 via Adafruit DHT library
│   ├── BH1750Sensor            I2C light sensor
│   └── PhotoresistorSensor     analog (A1), alternative to BH1750
└── Actuator
    ├── WaterPump               relay on D3
    ├── VentilationFan          L298-style H-bridge on D4/D5/D6
    ├── LEDGrowLight            PWM on D9
    └── ESP32CAM                SoftwareSerial on D10/D11
```

Singletons stitch it together:

- `ModuleManager` — auto-detects which modules respond to `init()`, exposes
  them by `ModuleType` enum, only updates available ones in `loop()`.
- `Settings` — runtime-mutable thresholds + mode. Persisted to EEPROM
  (debounced to protect flash wear). Restored on every boot.
- `DisplayManager` — SSD1306 OLED, two pages of system status. Button on D8
  cycles pages.
- `GreenhouseServer` — WiFi REST API on port 80; dashboard polls it.

## REST API

All `/api/*` endpoints return JSON and include CORS headers.

| Endpoint | Method | Description |
| --- | --- | --- |
| `/` | GET | Plain HTML info page (visit from a browser to confirm the device is up) |
| `/api/state` | GET | Snapshot of sensors, actuators, thresholds, mode, available modules |
| `/api/actuator?id=pump\|fan\|led&state=on\|off` | POST | Turn an actuator on/off (only takes effect in manual mode) |
| `/api/mode?value=auto\|manual` | POST | Switch automation mode |
| `/api/threshold?key=soilDry\|soilWet\|hum\|temp\|lux&value=<n>` | POST | Update a runtime threshold (persisted to EEPROM after a 5 s debounce) |
| `/api/snapshot` | POST | Trigger ESP32-CAM image capture |
| `*` | OPTIONS | CORS preflight |

### Authentication

Set `API_TOKEN` in `include/secrets.h` to any non-empty string and the server
will require header `X-Auth-Token: <value>` on every `/api/*` request. The
dashboard reads its copy of the token from the top-bar input and sends it
automatically. Leave `API_TOKEN` empty for open LAN access (defaults).

## First-time setup

1. **Copy secrets template**: `cp include/secrets.h.example include/secrets.h`,
   then fill in `WIFI_SSID`, `WIFI_PASSWORD`, and optionally `API_TOKEN`.
2. **Edit `include/config.h`** if your wiring differs — pins, sensor variant
   (`USE_BH1750_LIGHT_SENSOR` vs `USE_PHOTORESISTOR_LIGHT_SENSOR`), DHT
   variant (`DHT_SENSOR_TYPE` 11 or 22), thresholds.
3. **Build + upload**:

   ```bash
   pio run -t upload -e uno_r4_wifi
   ```

4. **Monitor serial**:

   ```bash
   pio device monitor -b 9600
   ```

   Look for `[Net] Connected. http://192.168.x.x` — that's the dashboard host.
5. **Open the dashboard**: open `index.html` in a browser. Paste the IP from
   step 4 into the top-right `Host` input. If you set `API_TOKEN`, paste the
   same value into the `API token` input.

## Module enable/disable

In `config.h`:

```cpp
#define ENABLE_SOIL_MOISTURE_SENSOR 1
#define ENABLE_DHT_SENSOR           1
#define ENABLE_LIGHT_SENSOR         1
#define ENABLE_PUMP                 1
#define ENABLE_FAN                  1
#define ENABLE_LED_GROW_LIGHT       1
#define ENABLE_ESP32_CAM            1
```

A disabled module is never created. An enabled-but-unresponsive module is
detected during `init()` and the control loop skips it via
`isModuleAvailable()`.

## Pin map

| Component        | Pin(s)       | Type                    |
| ---------------- | ------------ | ----------------------- |
| Soil Moisture    | A0           | Analog Input            |
| Photoresistor    | A1           | Analog Input            |
| DHT11/22         | D2           | Digital Signal          |
| Water pump relay | D3           | Digital Output          |
| Fan H-bridge     | D4 / D5 / D6 | IN1 / IN2 / EN          |
| OLED button      | D8           | Digital Input (pull-up) |
| LED grow light   | D9           | PWM                     |
| ESP32-CAM        | D10 / D11    | Soft-Serial RX/TX       |
| BH1750 / OLED    | A4 / A5      | I2C SDA / SCL           |

## Control logic

In `MODE_AUTO` (default), `loop()` mirrors actuator state to sensor readings:

- **Pump**: hysteresis between `soilDryThreshold` and `soilWetThreshold` —
  on when dry, off when wet. Polarity auto-detected.
- **Fan**: on when `temp > tempHigh` OR `hum > humidityHigh`. Safety
  cutoff at `FAN_MAX_ON_TIME`.
- **LED**: on at brightness 200/255 when light reading is below
  `lightLow` (BH1750) or above `lightLow` (photoresistor — inverted).
- **Camera**: auto-capture every 10 minutes (set once in `setup()`).

In `MODE_MANUAL` the dashboard owns the actuators via `/api/actuator`; the
automation loop is skipped. Switching back to `MODE_AUTO` resumes
sensor-driven control on the next tick. Mode is persisted.

## Safety guarantees

- `PUMP_MAX_ON_TIME` and `FAN_MAX_ON_TIME` in `config.h` force-off any
  actuator that's been running too long.
- Actuator `on()` is idempotent — a repeat call neither resets the safety
  timer nor logs noise.
- EEPROM writes are debounced (5 s after last change) to limit flash wear.

## File layout

```text
PPBL/
├── include/
│   ├── config.h, Module.h, Sensor.h, Actuator.h
│   ├── Settings.h            runtime config (mode + thresholds, EEPROM-backed)
│   ├── *Sensor.h, *Pump.h …  device drivers
│   ├── DisplayManager.h, ModuleManager.h, GreenhouseServer.h
│   ├── secrets.h.example     copy to secrets.h (gitignored)
│   └── secrets.h             local credentials (WiFi + API_TOKEN)
├── src/
│   └── … one .cpp per .h, plus main.cpp
├── lib/StepperMotor/         archived driver, not in build
├── index.html                stand-alone web dashboard
├── platformio.ini
└── README.md
```

## Adding a new module

1. Create `include/MyModule.h` inheriting from `Sensor` or `Actuator`.
2. Implement `init()`, `isAvailable()`, `update()`, `getName()`,
   `getModuleType()` (new value in the `ModuleType` enum).
3. Add the file pair under `include/` and `src/`.
4. Register in `ModuleManager::createEnabledModules()` behind an
   `#if ENABLE_…` guard.
5. (Optional) wire it into `updateControlLogic()` in `main.cpp` and
   `serveState()` in `GreenhouseServer.cpp`.

## Troubleshooting

- **`[DHTSensor] FAILED to detect`**: check 4.7 kΩ pull-up between DATA and VCC,
  confirm DHT_SENSOR_TYPE matches the actual sensor (11 or 22).
- **`[Net] WiFi connect FAILED`**: SSID/password in `secrets.h`, 2.4 GHz only.
- **Dashboard shows `unauthorized`**: API token in the top-bar must match
  `API_TOKEN` in `secrets.h`.
- **Thresholds revert after reboot**: check the serial log for
  `[Settings] Loaded from EEPROM` — if it says "EEPROM checksum mismatch",
  the flash blob is corrupted (rare; first write fixes it).
