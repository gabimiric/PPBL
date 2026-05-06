# Autonomous Plant Care System - Wiring Guide

Complete wiring diagram and connection instructions for the PPBL plant care system using Arduino UNO R4 WiFi and HW131 power supply module.

## Power Supply Setup (HW131 Module)

The HW131 power supply module has been configured with **two output rails**:

- **Rail 1: 3.3V** (for low-voltage components)
- **Rail 2: 5V** (for main system)

### HW131 Input

Connect your main power source (USB, 12V adapter, etc.) to the HW131 input terminals.

### HW131 Output Distribution

#### 5V Rail (Primary Power)

- **+5V** → Arduino VIN pin (power input, NOT the 5V output pin)
- **+5V** → H-Bridge logic VCC (if your board requires 5V logic supply)
- **+5V** → Relay module +5V (if used)
- **GND** → Arduino GND pin (CRITICAL - common ground)
- **GND** → H-Bridge GND pin (CRITICAL - common ground)
- **GND** → Relay module GND (if used)

#### 3.3V Rail

- Reserved for future use or low-voltage sensors

---

## Component Pin Assignments

| Component           | Type           | Arduino Pin | Notes                      |
| ------------------- | -------------- | ----------- | -------------------------- |
| DHT11 Sensor        | Digital Input  | 2           | Temperature/Humidity       |
| Water Pump Relay    | Digital Output | 3           | Active HIGH                |
| Fan H-Bridge IN1    | Digital Output | 4           | DC motor direction input   |
| Fan H-Bridge IN2    | Digital Output | 5           | DC motor direction input   |
| Fan H-Bridge EN/PWM | Digital Output | 6           | PWM speed control (ENA/EN) |
| Display Page Button | Digital Input  | 8           | Switch OLED pages          |
| LED Grow Light      | Digital Output | 9           | PWM capable                |
| ESP32CAM RX         | Digital Input  | 10          | Serial communication       |
| ESP32CAM TX         | Digital Output | 11          | Serial communication       |
| Soil Moisture       | Analog Input   | A0          | 0-1023 value               |
| Photoresistor (LDR) | Analog Input   | A1          | 0-1023 value               |
| I2C SDA             | I2C            | A4          | For BH1750 & OLED          |
| I2C SCL             | I2C            | A5          | For BH1750 & OLED          |

---

## Detailed Sensor Connections

### 1. DHT11 Temperature/Humidity Sensor (Pin 2)

```
DHT11 Module:
  [VCC] ────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [Data] ───────────→ Arduino Pin 2
  [NC] ─────────────→ (Not Connected)
```

**Note:** If using a bare DHT11 (not module), add a 4.7kΩ pull-up resistor between Data pin and 5V.

---

### 2. Soil Moisture Sensor (Pin A0)

```
Soil Moisture Analog Sensor:
  [VCC] ────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [A0] ─────────────→ Arduino Pin A0
```

**Calibration Notes:**

- DRY reading (typical): ~700-1023
- WET reading (typical): ~0-300
- Configure thresholds in config.h: `SOIL_MOISTURE_DRY_THRESHOLD` and `SOIL_MOISTURE_WET_THRESHOLD`

---

### 3. Photoresistor (LDR) Light Sensor (Pin A1)

```
                    5V (HW131 Rail 2)
                         |
                    [10kΩ Resistor]
                         |
                    ┌────┴────┬──────→ Arduino Pin A1
                    |         |
                  [LDR]       |
                  (Photo      |
                 resistor)    |
                    |         |
                    └────┬────┘
                         |
                    GND (HW131 Rail 2)

Voltage divider configuration:
- Top: 10kΩ resistor (between 5V and A1)
- Bottom: Photoresistor (between A1 and GND)
- Analog pin A1 reads the junction
```

**Light Level Mapping:**

- Dark room: ~0-200
- Dim indoor light: ~200-500
- Normal indoor light: ~500-800
- Bright sunlight: ~800-1023
- Configure threshold in config.h: `LIGHT_LEVEL_THRESHOLD` (default: 500)

---

### 4. Water Pump Relay (Pin 3)

```
Pump Relay Module (typically 2-channel or single):
  [VCC] ────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [IN] ─────────────→ Arduino Pin 3

Relay Output (NO - Normally Open) - Controls Pump Power:
  [COM] ────────────→ 5V (HW131 Rail 2)
  [NO] ─────────────→ Water pump positive terminal
  [Pump GND] ───────→ GND (HW131 Rail 2)
```

**Note:** Pump power comes directly from HW131 5V rail. The relay acts as a switch to control when the pump receives power.

---

### 5. DC Ventilation Fan via H-Bridge (Pins 4-6)

```
H-Bridge Module (L298N/L293D/TB6612 style):

  ARDUINO CONTROL INPUTS:
  [IN1] ────────────→ Arduino Pin 4
  [IN2] ────────────→ Arduino Pin 5
  [EN/ENA/PWMA] ────→ Arduino Pin 6

  POWER INPUTS:
  [VCC logic] ──────→ 5V (HW131 Rail 2, if required by your board)
  [GND] ────────────→ GND (HW131 Rail 2)
  [VM/Vmotor] ──────→ 5V (HW131 Rail 2 for 5V fan)

  MOTOR CONNECTION:
  [OUT1/AO1] ───────→ Fan motor terminal 1
  [OUT2/AO2] ───────→ Fan motor terminal 2
```

**Fan Behavior in Current Firmware:**

- IN1=HIGH, IN2=LOW -> fan ON (forward)
- IN1=LOW, IN2=LOW -> fan OFF
- EN pin uses PWM (0-255), currently set to full speed when ON

**⚠️ CRITICAL:** Do not power the fan directly from an Arduino GPIO pin. Use the H-bridge outputs. Keep all grounds common (Arduino, H-bridge, fan supply).

---

### 6. LED Grow Light (Pin 9)

```
LED Module:
  [VCC] ────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [In/PWM] ─────────→ Arduino Pin 9 (PWM capable)
```

**Features:**

- PWM brightness control: 0-255 (0% = off, 255 = 100%)
- Current draw: ~50-200mA depending on type
- Photoperiod: Configurable 16 hours ON / 8 hours OFF (default)

---

### 7. ESP32CAM Camera Module (Pins 10-11 Serial)

```
ESP32CAM Module:
  [5V] ─────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [RX] ─────────────→ Arduino Pin 10 (SoftwareSerial)
  [TX] ─────────────→ Arduino Pin 11 (SoftwareSerial)
```

**Communication:**

- Serial protocol: 9600 baud (configurable)
- Interface: SoftwareSerial on pins 10-11
- Requires separate 5V power supply with at least 500mA capacity

---

### 8. OLED Display 64x128 SSD1306 (I2C)

```
OLED SSD1306 Module:
  [VCC] ────────────→ 5V (HW131 Rail 2)
  [GND] ────────────→ GND (HW131 Rail 2)
  [SDA] ────────────→ Arduino A4 (I2C Data)
  [SCL] ────────────→ Arduino A5 (I2C Clock)
```

**Characteristics:**

- Display size: 128x64 pixels
- Interface: I2C (0x3C default address, may vary 0x3C or 0x3D)
- Current draw: ~20-30mA
- Voltage: 5V (can tolerate 3.3V for data pins with level shifter)
- Update rate: 100ms refresh in software
- Page switching: Button on Arduino Pin 8

**Status Display:**

- Page 1: Uptime, Soil moisture, Temperature & Humidity, Light Level
- Page 2: Pump status, Fan status, LED status
- Toggle with button press (200ms debounce)

### 9. Display Page Button (Pin 8)

```
Button/Switch:
  [1] ────────────→ Arduino Pin 8
  [2] ────────────→ GND (HW131 Rail 2)
```

**Characteristics:**

- Momentary pushbutton (normally open)
- Press to switch between OLED pages (Page 1 ↔ Page 2)
- Pull-up resistor: Internal (INPUT_PULLUP)
- Debounce: 200ms hardware debounce

---

## Complete System Wiring Summary

### Arduino Pins Used

```
Digital Output:  3, 4, 5, 6, 9
Digital Input:   2, 10, 11
Analog Input:    A0, A1
I2C Bus:         A4 (SDA), A5 (SCL)
Power:           5V, GND
```

### Power Distribution Chain

```
Main Power Source (USB/12V/Battery)
    ↓
HW131 Power Supply Module
    ├─→ 5V Rail ────┬─→ Arduino VIN (or Arduino USB)
    │               ├─→ DHT11 Sensor (5V)
    │               ├─→ Soil Moisture Sensor (5V)
    │               ├─→ Photoresistor Circuit (5V)
    │               ├─→ OLED Display SSD1306 (5V)
    │               ├─→ Pump Relay (5V control)
    │               ├─→ Water Pump (5V, via relay)
    │               ├─→ H-Bridge Driver (5V logic + motor supply)
    │               ├─→ DC Ventilation Fan (via H-bridge)
    │               ├─→ LED Grow Light (5V)
    │               └─→ ESP32CAM (5V)
    │
    └─→ 3.3V Rail (reserved for future use)

⚠️ CRITICAL: Arduino GND ←→ HW131 GND (common ground for all components)
```

---

## Critical Electrical Rules

### ⚠️ COMMON GROUND REQUIREMENT

**All components must share a common ground:**

```
HW131 GND ────┬─→ Arduino GND
              ├─→ All sensor GND pins
              ├─→ All actuator GND pins
              └─→ Pump/external power supply GND
```

Without common ground, sensors may not read correctly and control signals will fail.

### Power Capacity

Ensure your HW131 input power supply can provide:

- Arduino board: ~100-150mA
- All sensors (DHT, soil, LDR): ~50mA
- LED grow light: ~100-200mA
- DC fan motor: ~100-300mA (depends on fan model)
- Water pump: ~200-500mA (depends on pump)
- **Total: ~0.8-1.2A** (use 2A+ power supply for headroom)

**All components should be powered from the HW131 5V rail, not the Arduino's internal regulators.** This ensures stable voltage and sufficient current for all devices.

---

## Testing Checklist

- [ ] HW131 is powered and 5V LED is on
- [ ] Arduino GND is connected to HW131 GND
- [ ] All connections are secure (no loose wires)
- [ ] No shorts between power and ground rails
- [ ] DHT11 reading: 20-30°C, 30-60% humidity (typical indoor)
- [ ] Soil moisture: 0-1023 range detected
- [ ] Photoresistor: Changes with light (0 in darkness, 800+ in bright light)
- [ ] LED light: Brightens when light level is low
- [ ] Fan motor: Spins when humidity/temperature threshold is exceeded
- [ ] Water pump relay: Clicks when activated
- [ ] Water pump: Turns on when relay is activated
- [ ] Serial monitor shows initialization messages for all modules

---

## Troubleshooting

| Issue                                  | Cause                            | Solution                                                          |
| -------------------------------------- | -------------------------------- | ----------------------------------------------------------------- |
| Sensors not reading                    | No common ground                 | Connect all GND to HW131 GND                                      |
| Fan motor won't spin                   | H-bridge wiring/power issue      | Verify IN1/IN2/EN pins (4/5/6), VM supply, and common GND         |
| Fan spins opposite direction           | Motor leads reversed             | Swap motor wires on OUT1/OUT2                                     |
| DHT11 timeout errors                   | Bad connection/pin               | Verify pin 2, check 4.7kΩ pull-up (if needed)                     |
| Pump relay clicks but pump doesn't run | Relay COM not connected to power | Check relay COM is connected to HW131 5V, and NO connects to pump |
| LED not working                        | Wrong pin or supply              | Verify pin 9, check 5V supply from HW131                          |

---

## Configuration Changes

To enable/disable components, edit `include/config.h`:

```cpp
#define ENABLE_SOIL_MOISTURE_SENSOR 1  // 1=ON, 0=OFF
#define ENABLE_DHT_SENSOR 1
#define ENABLE_LIGHT_SENSOR 1
#define ENABLE_PUMP 1
#define ENABLE_FAN 1
#define ENABLE_LED_GROW_LIGHT 1
#define ENABLE_ESP32_CAM 1

#define FAN_HBRIDGE_IN1_PIN 4
#define FAN_HBRIDGE_IN2_PIN 5
#define FAN_HBRIDGE_EN_PIN 6

// Light sensor type (choose ONE):
#define USE_BH1750_LIGHT_SENSOR 0          // 0=disabled
#define USE_PHOTORESISTOR_LIGHT_SENSOR 1   // 1=enabled
```

---

## Revision History

| Date       | Changes                                        |
| ---------- | ---------------------------------------------- |
| 2026-05-06 | Initial wiring guide with HW131 power module   |
|            | Changed from StepperMotor to DC fan (H-bridge) |
|            | Added photoresistor sensor                     |
|            | All power from HW131 5V rail                   |
