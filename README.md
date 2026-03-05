# Tesseract — WWVB 60 kHz Time Signal Transmitter

A low-power ESP32-based WWVB transmitter that synchronizes "atomic" clocks indoors by rebroadcasting the 60 kHz time signal using NTP as the time source.

## Problem

Five WWVB "atomic" clocks in a Seattle home can't reliably receive the Fort Collins, CO transmitter (~800 miles). They only sync after extended rooftop exposure, then drift several minutes over 2-3 months.

## Solution

An ESP32 acquires accurate UTC time via NTP from local stratum-1 GPS-disciplined servers, encodes it into the WWVB amplitude-modulated format, and transmits at 60 kHz through a ferrite rod antenna — continuously, 24/7, within FCC Part 15 limits.

## Hardware

| Component | Spec | Cost |
|-----------|------|------|
| ESP32 DevKit | HiLetgo ESP32-WROOM-32D ([Amazon B08D5ZD528](https://www.amazon.com/gp/product/B08D5ZD528)) | ~$6 |
| Ferrite Rod | 10mm × 200mm, material 61 or 77 | ~$12 |
| Magnet Wire | 30 AWG enameled copper, ~50 feet | ~$9 |
| Transistor | 2N2222 NPN | ~$2 |
| Base Resistor | 1kΩ | — |
| Current Limit Resistor | 100Ω (start conservative) | — |
| **Total** | | **~$29** |

### Circuit

```
         5V (USB)
          │
          R2 (100Ω current limit)
          │
     Ferrite Coil (200 turns, 30 AWG)
          │
          Collector
GPIO 18 ──[R1 1kΩ]── Base   2N2222
                      Emitter
                        │
                       GND
```

### Antenna Construction

1. Wind 200 turns of 30 AWG magnet wire tightly on the ferrite rod
2. Mark every 50 turns with tape for counting
3. Leave 6" of wire free at each end
4. Secure coil with tape or clear nail polish
5. Scrape enamel off wire ends for electrical contact

## Software Setup

### Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) with ESP32 board support
- Board: **ESP32 Dev Module**
- Baud rate: **115200**

### Install ESP32 Board Support
1. Arduino IDE → File → Preferences
2. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Tools → Board → Boards Manager → Search "esp32" → Install

### Configure

Edit `wwvb_transmitter/config.h` with your settings:

```cpp
const char* WIFI_SSID     = "your-ssid";
const char* WIFI_PASSWORD = "your-password";
const char* NTP_SERVER_1  = "192.168.1.100";   // Your local NTP server
const char* NTP_SERVER_2  = "192.168.1.101";   // Backup NTP server
const char* NTP_SERVER_3  = "pool.ntp.org";     // Internet fallback
```

### Upload

1. Open `wwvb_transmitter/wwvb_transmitter.ino` in Arduino IDE
2. Select **ESP32 Dev Module** as the board
3. Select the correct COM port
4. Click Upload
5. Open Serial Monitor at 115200 baud

## Testing

Four standalone test sketches validate each component independently. Upload each `.ino` to the ESP32 and read results in Serial Monitor.

### 1. Encoder Unit Tests (No Hardware Needed)
```
tests/test_wwvb_encoder/
```
Copy `wwvb_encoder.h` and `wwvb_encoder.cpp` from `wwvb_transmitter/` into this folder, then upload. Runs 100+ tests covering BCD encoding, marker placement, reserved bits, time rollovers, DST logic, leap year handling, and reference frame validation.

**Run this first** — validates all encoding logic before touching hardware.

### 2. NTP Client Tests (WiFi Required)
```
tests/test_ntp_client/
```
Edit WiFi credentials in the sketch, then upload. Tests WiFi connection, NTP sync, time monotonicity, UTC offset verification, second boundary precision, and WiFi reconnection.

### 3. PWM Output Tests (Oscilloscope Required)
```
tests/test_pwm_output/
```
Connect oscilloscope to GPIO 18. Upload and step through interactive tests: continuous 60 kHz carrier, on/off transitions, and bit timing verification (200ms/500ms/800ms pulses).

### 4. Hardware Validation (Full System)
```
tests/hardware_validation/
```
Full integration test: NTP sync → WWVB encode → transmit. Runs continuously with per-bit diagnostics, timing accuracy tracking, and system health monitoring. Place a WWVB clock near the antenna to test reception.

## Project Structure

```
tesseract/
├── README.md                              # This file
├── .clinerules/
│   └── objective.md                       # Project rules for AI assistants
├── specs/                                 # Detailed technical documentation
│   ├── architecture.md                    # System architecture & decisions
│   ├── wwvb_protocol.md                   # WWVB encoding specification
│   ├── hardware_specs.md                  # ESP32, antenna, circuit details
│   ├── fcc_compliance.md                  # FCC Part 15 requirements
│   ├── ntp_integration.md                 # NTP client design & failover
│   ├── testing_strategy.md                # Test plan & acceptance criteria
│   ├── implementation_phases.md           # Development phases & milestones
│   └── references.md                      # External links & datasheets
├── wwvb_transmitter/                      # Main Arduino sketch
│   ├── wwvb_transmitter.ino               # Entry point
│   ├── config.h                           # WiFi, NTP, pin configuration
│   ├── wwvb_encoder.h / .cpp              # WWVB time code encoder
│   ├── ntp_manager.h / .cpp               # NTP sync with failover
│   └── debug_utils.h                      # Serial & LED helpers
└── tests/                                 # Standalone test sketches
    ├── test_wwvb_encoder/                 # Encoder unit tests
    ├── test_ntp_client/                   # NTP integration tests
    ├── test_pwm_output/                   # PWM/oscilloscope tests
    └── hardware_validation/               # Full system integration
```

## How It Works

1. **NTP Sync** — ESP32 connects to WiFi and syncs UTC time from local stratum-1 GPS-disciplined servers (sub-millisecond accuracy on LAN)
2. **WWVB Encode** — At the start of each minute, encodes the *next* minute's time into a 60-bit frame using BCD with position markers
3. **Transmit** — Each second, modulates the 60 kHz carrier by turning it off for 200ms (binary 0), 500ms (binary 1), or 800ms (marker), then back on for the remainder
4. **Receive** — WWVB clocks detect the amplitude changes and decode the time code

## FCC Compliance

This device operates under [FCC Part 15.209](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/section-15.209) — intentional radiator below 490 kHz. The limit is 40 μV/m at 300 meters at 60 kHz. The ferrite rod antenna is electrically tiny (wavelength = 5 km), making it inherently inefficient. Combined with low drive current and building attenuation, the transmitted signal stays well below FCC limits. See [`specs/fcc_compliance.md`](specs/fcc_compliance.md) for the full analysis.

## DST Configuration

WWVB clocks use DST status bits to display local time. Set `currentDSTStatus` in `wwvb_transmitter.ino`:

| Value | Meaning |
|-------|---------|
| `DST_STANDARD` | Standard Time in effect |
| `DST_BEGINS` | DST begins today |
| `DST_IN_EFFECT` | DST in effect |
| `DST_ENDS` | DST ends today |

*Future enhancement: automatic DST calculation based on timezone rules.*

## Troubleshooting

| Issue | Check |
|-------|-------|
| Clock won't sync | Verify 60 kHz on oscilloscope; try closer range; check antenna orientation (broadside to rod = strongest) |
| WiFi won't connect | Verify SSID/password in `config.h`; check Serial Monitor for error messages |
| NTP sync fails | Verify NTP server IPs; try `pool.ntp.org` as primary; check firewall |
| Wrong time on clock | Verify UTC encoding (not local time); check DST status bits |
| No carrier output | Check GPIO 18 connection; verify PWM with `test_pwm_output` sketch |
| Short range | Reduce R2 (current limit); add resonant capacitor; verify coil winding |

## License

This project is for personal/educational use. WWVB signal format is defined by NIST. Ensure compliance with local radio regulations before operating.

## References

- [NIST WWVB](https://www.nist.gov/pml/time-and-frequency-division/radio-stations/wwvb) — Official WWVB documentation
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) — ESP32 board support
- [FCC Part 15.209](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/section-15.209) — Field strength limits
- See [`specs/references.md`](specs/references.md) for complete reference list
