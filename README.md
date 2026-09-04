# Sailboat IoT Telemetry Station

Autonomous, battery-powered telemetry unit for a sailboat. It monitors ambient
temperature, humidity, and bilge water level, and reports the readings over a
2G/GPRS cellular connection three times a day. The system is designed to run
for weeks on a single 18650 Li-ion cell by spending nearly all of its time in
deep sleep, waking on a timer or immediately on a bilge water alert.

![Build photo placeholder](docs/images/build-overview.jpg)
*Add a photo of the current build here.*

---

## Table of Contents

1. [Features](#features)
3. [Hardware Components](#hardware-components)
4. [Component Selection Rationale](#component-selection-rationale)
5. [Power Architecture](#power-architecture)
6. [Pin Mapping](#pin-mapping)
7. [Firmware](#firmware)
8. [Development Environment Setup](#development-environment-setup)
9. [Progress Log](#progress-log)
10. [Known Issues](#known-issues)
11. [Roadmap](#roadmap)
12. [License](#license)

---

## Features

- Temperature, humidity, and barometric context via I2C sensor
- Bilge water level monitoring via float switch, with hardware interrupt for
  immediate wake-up on flooding, independent of the sleep timer
- Cellular data reporting (2G/GPRS) three times a day, minimizing radio-on time
- Deep sleep power management: dual wake source (timer + external interrupt)
- Fully self-powered: single 18650 Li-ion cell, no connection to the boat's
  electrical system
- Designed for a marine environment: sealed/vented enclosure, waterproof cable
  entries, corrosion-aware sensor placement

---

## Hardware Components

| Component | Role | Datasheet | Photo |
|---|---|---|---|
| ESP32 NodeMCU-32S (38-pin, CP2102) | Main controller | *add link* | <img src="images/NodemcuEsp32.webp" width="100" alt="ESP32"> |
| HTU21D | Temperature & humidity sensor (I2C) | *add link* | <img src="images/Htu21d.webp" width="100" alt="HTU21D"> |
| SIM800L | 2G/GPRS cellular module | *add link* | <img src="images/Sim800l .webp" width="100" alt="SIM800L"> |
| Float switch 6017-1P (dry contact) | Bilge water level sensor | *add link* | <img src="images/Switch.webp" width="100" alt="Float Switch"> |
| TP4056 | Li-ion charging module | *add link* | <img src="images/Tp4056.webp" width="100" alt="TP4056"> |
| XL6009 | Adjustable DC-DC step-up (boost) | *add link* | <img src="images/StepUp.webp" width="100" alt="XL6009"> |
| LM2596 | Adjustable DC-DC step-down (buck) | *add link* | <img src="images/Step-down.webp" width="100" alt="LM2596"> |
| 18650 Li-ion cell | Main power source | *add link* | <img src="images/18650.png" width="100" alt="18650"> |
| 1000µF 25V electrolytic capacitor | Absorbs SIM800L current spikes | *add link* | <img src="images/Capacitor.webp" width="100" alt="Capacitor"> |
| 10kΩ / 20kΩ resistors | UART voltage divider + pull-up | *add link* | <img src="images/10k.webp" width="100" alt="10K"> |

---

## Component Selection Rationale

**ESP32 (NodeMCU-32S) over Arduino or Raspberry Pi Zero.** Native deep sleep
with timer and external-interrupt wake sources, low active-mode power draw,
two hardware UARTs (needed for the SIM800L), 3.3V logic matching the sensors,
and enough RAM/flash to handle AT-command parsing and JSON payload assembly
without the overhead of a full Linux boot each cycle. A Raspberry Pi Zero,
while capable of the same task in software, has no real hardware-level power-off
between cycles and adds unnecessary boot latency for a job this small.

**HTU21D over BME280 / DHT22.** BME280/BMP280 lookalikes are commonly
mislabeled by resellers, and BMP280 specifically drops humidity entirely.
HTU21D shares the same I2C bus as the rest of the system (no extra GPIO/pull-up
needed like the 1-wire DHT22), and offers ±2% RH accuracy, adequate for a
weather-station use case.

**SIM800L (2G/GPRS) over a 4G module.** At the time of purchase, 2G networks
were still active in Argentina with no public shutdown date announced. The
data payload per transmission is tiny (a few sensor readings, 3 times a day),
well within GPRS bandwidth. A 4G-capable module (e.g. SIM7600/A7670) would have
been 5-10x more expensive for no practical benefit at this data volume.

**Float switch (dry contact) over a capacitive/resistive water sensor.**
Dry contact float switches are the marine-industry standard (same principle
used in automatic bilge pumps), with no exposed electronics in contact with
water, and no corrosion-prone traces like the cheap resistive water sensor
boards common in hobbyist kits.

**Boost (XL6009) + buck (LM2596) chain over a single regulator.** A single
18650 cell (3.0-4.2V) cannot directly supply a stable 4.0V for the SIM800L
across its full discharge range, and the LM2596 buck regulator requires at
least ~1.5V of headroom between input and output to regulate correctly. The
chain steps the battery voltage up to a fixed ~6.0V first (XL6009), then down
to a clean 4.0V for the SIM800L (LM2596) — giving comfortable headroom at
every stage regardless of the battery's charge state.

**Single 18650 cell over 2 cells in series.** Two cells in series would
require a balancing BMS to charge safely, which the simple TP4056 charger
does not provide. A single cell keeps the charging circuit simple (plain
TP4056) while still providing an estimated 1-2 months of autonomy given the
system's mostly-sleeping duty cycle.

---

## Pin Mapping

### HTU21D (I2C)
| HTU21D pin | ESP32 pin |
|---|---|
| VIN | 3V3 |
| GND | GND |
| DA (SDA) | GPIO 21 |
| CL (SCL) | GPIO 22 |

### Bilge float switch (dry contact)
| Wire | ESP32 pin |
|---|---|
| Wire 1 | GND |
| Wire 2 | GPIO 33 (RTC GPIO, internal pull-up enabled in firmware) |

### SIM800L (UART2)
| SIM800L pin | Connection |
|---|---|
| VCC | LM2596 output (4.0V) + capacitor positive lead |
| GND | LM2596 GND + capacitor negative lead + common ground |
| TXD | ESP32 GPIO 16 (RX2) — direct connection |
| RXD | ESP32 GPIO 17 (TX2) through a voltage divider (10kΩ series + 20kΩ to GND) to bring 3.3V logic down to ~2.2V |

---

## Firmware

Current test sketch: [`ESP32-Script.cpp`](/ESP32-Script.cpp)

What it currently does:
- Prints the deep sleep wake-up reason (timer vs. external interrupt) over serial
- Initializes I2C and attempts to read the HTU21D
- Reads the bilge float switch state
- Re-arms both wake sources (timer + EXT0) and re-enters deep sleep

Libraries used:
- [Adafruit HTU21DF Library](https://github.com/adafruit/Adafruit_HTU21DF_Library)

Not yet implemented in firmware:
- SIM800L AT command handling
- Cellular data transmission / payload formatting
- Production sleep interval (currently 30 seconds for bench testing, will be
  changed to match the 3x/day schedule)

---

## Development Environment Setup

1. **Arduino IDE** — [Download](https://www.arduino.cc/en/software/)
   Install the ESP32 board package from Espressif Systems via
   *Boards Manager* inside the IDE.

2. **CP210x USB-to-UART driver** (Silicon Labs) — required for the
   NodeMCU-32S board to be recognized as a COM port on Windows —
   [Download](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads)

3. **Adafruit HTU21DF library** — install via
   *Sketch > Include Library > Manage Libraries* inside the Arduino IDE,
   search "Adafruit HTU21DF".

---

## Progress Log

- [x] Arduino IDE configured with ESP32 board package
- [x] CP210x driver installed, COM port recognized correctly on Windows
- [x] Adafruit HTU21DF library installed
- [x] Deep sleep test firmware flashed and running
- [x] Dual wake source validated: timer wake and external interrupt (EXT0)
      via bilge float switch both confirmed working in serial logs
- [x] Float switch wiring validated: GND + GPIO 33, correctly reports both
      "dry" and "water alert" states
- [x] HTU21D header pins (VIN, GND, DA, CL) soldered
- [x] SIM800L coiled antenna soldered to the NET pad
- [ ] HTU21D not yet reading correctly — see Known Issues
- [ ] SIM800L AT command / data transmission firmware — not started
- [ ] Final component purchases (battery, holder, resistors, jumper wires, SIM card)
- [ ] DC-DC converter voltage calibration with multimeter
- [ ] Full system integration test

See [`/logs`](logs) for raw serial monitor captures from bench testing.

---

## Known Issues

### HTU21D not detected (unresolved)

Every serial log captured so far — across both timer wake-ups and float
switch alert wake-ups — shows:

```
[ERROR] No se detecto el sensor HTU21D. Revisa SDA y SCL.
```

The sensor has never successfully initialized in any test run. Things to
check next:
- Confirm SDA/SCL aren't swapped at the header pins (easy to mix up when
  hand-soldering a 4-pin strip)
- Re-check the solder joints on the HTU21D header with a magnifier / continuity
  test — a cold joint on VIN or GND would cause exactly this symptom
  intermittently or permanently
- Confirm the sensor is getting 3.3V at its VIN pin with a multimeter while
  the ESP32 is awake
- Try an I2C scanner sketch (independent of the HTU21D library) to confirm
  whether the device shows up at its expected address (0x40) on the bus at all
- Double check no other device on the bus is conflicting (shouldn't be an
  issue yet, since HTU21D is currently the only I2C device)

---

## Roadmap

- [ ] Debug and resolve HTU21D detection issue
- [ ] Purchase remaining components: 18650 cell (Samsung/LG/Panasonic,
      2600-3500mAh), battery holder, 10kΩ/20kΩ resistors, jumper wires,
      prepaid 2G/GPRS SIM card
- [ ] Calibrate XL6009 (6.0V) and LM2596 (4.0V) with a multimeter before
      connecting any load
- [ ] Implement SIM800L AT command routine and data upload (HTTP POST or
      similar) in firmware
- [ ] Switch sleep interval from 30s test value to production schedule
      (wake 3x/day)
- [ ] Full integration test: sensors + cellular transmission + deep sleep
      cycle, end to end
- [ ] Build and seal the enclosure (see `/enclosure`)
- [ ] Install and field-test aboard the boat
