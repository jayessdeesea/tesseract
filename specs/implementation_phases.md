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
1. Breadboard circuit: ESP32 → 1kΩ → 2N2222 → coil → 5V
2. Start with 100Ω current-limiting resistor (conservative)
3. Configure ESP32 LEDC PWM for 60 kHz output
4. Verify 60 kHz carrier with oscilloscope
5. Verify amplitude modulation (200ms/500ms/800ms pulses)

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

## Future Enhancements (Optional)
- Web interface showing status, uptime, NTP sync info
- OTA firmware updates
- Resonant circuit for improved range
- Multiple protocol support (DCF77, MSF, JJY)
- Automatic power control
