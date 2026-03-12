# Implementation Phases

## Phase 1: Code Development ✅ (Current)

### Tasks
1. Set up Arduino IDE with ESP32 board support
2. Implement WWVB encoder module (`wwvb_encoder.h/cpp`)
3. Implement NTP client with multi-server failover (`ntp_manager.h/cpp`)
4. Implement main sketch with timer interrupt (`wwvb_transmitter.ino`)
5. Write comprehensive test suite
6. Validate encoder against NIST reference frames

### Deliverables
- Complete Arduino sketch set in `wwvb_transmitter/`
- Test sketches in `tests/`
- All encoder unit tests passing

## Phase 1.5: Early Hardware Validation (Optional)

If the ferrite rod hasn't arrived, you can do early proof-of-concept testing with a **temporary air-core antenna**:

### Temporary Antenna
- Wind 100 turns of 30 AWG wire on a **cardboard tube** (~1" diameter)
- No ferrite core needed — range will be very short (2-6 inches)
- Connect via ULN2003AN driver circuit (same as Phase 3)
- Place WWVB clock directly next to the coil to test reception

### Value
- Validates the **complete signal chain** before ferrite rod arrives
- Confirms software encoding, timing, and modulation are correct
- Any issues found here are software/circuit, not antenna

## Phase 2: Antenna Construction

### Tasks
1. Wind 200 turns of 30 AWG wire on ferrite rod
2. Mark turn count every 50 turns (tape)
3. Secure coil ends with tape
4. Scrape enamel off wire ends
5. Measure inductance (if LCR meter available)
6. Calculate resonant capacitor (optional optimization)

### Deliverables
- Completed ferrite rod antenna
- Inductance measurement (if possible)

## Phase 3: Circuit Assembly

### Tasks
1. Breadboard circuit: ESP32 GPIO 18 → ULN2003AN pin 1, coil between pin 16 and +5V (through R2)
2. Connect ULN2003AN pin 8 to GND, pin 9 to +5V
3. Start with 100Ω current-limiting resistor (conservative)
4. Configure ESP32 LEDC PWM for 60 kHz output
5. Verify 60 kHz carrier with oscilloscope
6. Verify amplitude modulation (200ms/500ms/800ms pulses)

### Deliverables
- Working transmitter on breadboard
- Oscilloscope screenshots of 60 kHz carrier
- Oscilloscope screenshots of AM bit timing

## Phase 4: Testing and Range Verification

### Tasks
1. Place WWVB clock near transmitter, verify sync
2. Move clock to increasing distances (1ft, 5ft, 10ft, 20ft, 50ft)
3. Map house coverage (which rooms can sync)
4. Identify dead zones or problem areas
5. Iterate on antenna placement (elevation, orientation)
6. If needed: reduce R2, add more turns, or add resonant cap

### Deliverables
- Coverage map of house
- All five clocks syncing from installed locations
- Continuous operation verified (24+ hour test)

## Phase 5: Permanent Installation

### Tasks
1. Transfer from breadboard to protoboard or perfboard
2. Mount in enclosure (plastic project box)
3. Install in optimal location (central, elevated)
4. Secure antenna to prevent movement
5. Connect to permanent 5V USB wall adapter
6. Verify all clocks sync over 1-week period

### Deliverables
- Production unit in enclosure
- Installed and tested
- Documentation for future maintenance

## Phase 6: Status Display Interface

### Tasks
1. Wire four HT16K33 7-segment displays on I2C bus (GPIO 21/22)
2. Set I2C addresses (0x70, 0x71, 0x72, 0x73)
3. Install Adafruit LED Backpack and GFX libraries in Arduino IDE
4. Implement display manager module (`display_manager.h/cpp`)
5. Wire three status LEDs (NTP, WiFi, Transmit) with 220Ω resistors
6. Integrate display updates into main loop (every second)

### Display Layout
| Display | Address | Content | Example |
|---------|---------|---------|---------|
| 1 | 0x70 | Year | `2026` |
| 2 | 0x71 | Month.Day | `03.08` |
| 3 | 0x72 | Hour:Min (UTC) | `12:34` |
| 4 | 0x73 | Second + Status | `34.S` |

### LED Status Panel
- **🟢 GPIO 19:** NTP sync (solid = good, slow blink = aging, off = failed)
- **🔵 GPIO 23:** WiFi (solid = connected, fast blink = connecting)
- **🔴 GPIO 25:** Transmit heartbeat (brief flash each second)

### Deliverables
- Complete status display showing UTC date/time
- LED panel showing system health at a glance
- No serial monitor or computer needed for status monitoring

## Future Enhancements (Optional)
- Web interface showing status, uptime, NTP sync info
- OTA firmware updates
- Resonant circuit for improved range
- Multiple protocol support (DCF77, MSF, JJY)
- Automatic power control
