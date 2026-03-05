# WWVB Transmitter Design Document v1.0

## Document Purpose
This document captures the complete design for a home WWVB (60 kHz) time signal transmitter to synchronize atomic clocks in a Seattle residence. It is structured for AI consumption to provide full context for future implementation sessions.

## Project Overview

### Problem Statement
Five WWVB "atomic" clocks in a Seattle home cannot reliably sync with the Fort Collins, CO transmitter (~800 miles away) due to:
- Distance attenuation
- Building material shielding
- Indoor reception limitations

Clocks only sync after extended rooftop exposure, then drift several minutes over 2-3 months.

### Solution Approach
Build a low-power 60 kHz transmitter that:
1. Acquires accurate time from NTP (using local stratum-1 GPS-disciplined servers)
2. Encodes time into WWVB amplitude-modulated format
3. Transmits at 60 kHz within FCC Part 15 limits
4. Operates continuously (24/7) for always-available sync

### Design Constraints
- FCC Part 15.209 compliance: 40 μV/m @ 300m for frequencies below 490 kHz
- Whole-house coverage desired (multiple rooms, potentially multiple floors)
- Continuous operation required (clocks sync at unpredictable times)
- Prefer simplicity over precision (clocks display wall time, not distributed transactions)

## Architecture Decisions

### Rejected Approaches

#### Commercial Solution (Chronvertor)
**Rejected because:**
- Discontinued product, not available
- Battery-backed RTC approach (DS3231) drifts ~5 seconds/month
- Solves wrong problem (standalone time source) vs. actual problem (signal reception)
- Manual time setting or GPS sync adds complexity

**Lessons learned:**
- WWVB encoding logic is well-documented (can be adapted)
- Ferrite rod antenna approach is correct
- Carrier wave generation optional (not needed for direct-connect clocks)

#### Raspberry Pi + NTPd + Separate Oscillator
**Rejected because:**
- Linux is not real-time OS: scheduler jitter 100μs-10ms
- GPIO bit-banging has unpredictable latency (kernel delays, page faults)
- Cost: $60-80 for Pi 5 vs $6 for ESP32
- Power: 3-5W vs 0.5W
- Complexity: SD card corruption, OS updates, systemd
- Overkill: WWVB pulse timing needs ±50ms, not microsecond precision

**Why time precision doesn't matter:**
- WWVB pulse widths: 200ms, 500ms, 800ms
- Receivers need ±50ms accuracy
- NTP over local network provides ~1-10ms accuracy (100x better than needed)
- Even "bad" public NTP (±500ms) wouldn't break WWVB decoding

#### Multiple Transmitters (One Per Room)
**Rejected because:**
- **Beat frequency interference:** Crystal tolerance differences create 1-5 Hz amplitude beating, masking the 1 bit/second time code
- **Phase interference:** Constructive/destructive nulls throughout house
- **Sync nightmare:** Time code bits must align to exact millisecond across all units
- **FCC compliance:** Combined field strength matters, not per-device
- **Solution is simpler:** One centrally-located transmitter with proper antenna design

### Selected Architecture: ESP32 + NTP + Ferrite Rod Antenna

#### Core Components
1. **ESP32 DevKit** (ESP32-WROOM-32)
   - Dual-core 240 MHz, WiFi built-in
   - Hardware PWM (LEDC peripheral) for 60 kHz generation
   - ~$6 cost
   - Arduino IDE compatible

2. **NTP Time Source**
   - Primary: Local stratum-1 GPS-disciplined servers (192.168.1.x)
   - Secondary: Additional local stratum-1 servers (redundancy)
   - Tertiary: pool.ntp.org (internet fallback)
   - Sync interval: Every 1-6 hours
   - Accuracy: <10ms on local network

3. **Ferrite Rod Antenna**
   - 10mm diameter × 200mm length
   - Material 61 or 77 (high permeability at LF)
   - 200 turns of 30 AWG magnet wire
   - ~10-20 mH inductance

4. **Transistor Driver**
   - 2N2222 NPN (already in parts inventory)
   - Current limiting resistor
   - Drives antenna coil at 60 kHz

#### Why This Architecture Wins

**Time Accuracy:**
- NTP sync to local GPS-disciplined servers: sub-millisecond accuracy
- No drift between syncs (1-6 hour intervals)
- No manual intervention after WiFi setup
- Automatic handling of DST, leap seconds

**Timing Precision:**
- ESP32 hardware timer: sub-microsecond jitter
- FreeRTOS guarantees for interrupt priority
- LEDC PWM peripheral: hardware-generated 60 kHz, independent of CPU

**Simplicity:**
- Single device
- ~300 lines of Arduino C code
- No external RTC, GPS module, or complex RF
- Firmware updates over WiFi (OTA)

**Reliability:**
- Embedded system, no moving parts
- Watchdog timer auto-recovery
- No SD card corruption risk
- Power failure recovery: auto-reconnect and re-sync

**Cost:**
- ESP32: $6
- Ferrite rod: $12
- Magnet wire: $9
- Transistor/resistors: $2
- **Total: ~$29**

## Component Specifications

### ESP32 DevKit
- **Part:** HiLetgo ESP32-WROOM-32D DevKit (Amazon B08D5ZD528)
- **Features:**
  - ESP32-WROOM-32D/E chip
  - CP2102 USB-to-serial
  - 3.3V regulator
  - 38-pin breadboard layout
  - Micro-USB
- **Power:** 80mA idle + WiFi, ~100-150mA transmitting
- **GPIO:** All 3.3V (not 5V tolerant)
- **PWM:** Use GPIO 16-33 (avoid strapping pins)

### Ferrite Rod
- **Ordered:** 10mm × 200mm ferrite rod
- **Delivery:** ~1 month (acceptable, allows code development)
- **Alternative sources:**
  - Salvage from AM radio (Goodwill/thrift)
  - Mouser: Fair-Rite 2661480002 (material 61)
- **Why 200mm > 100mm:**
  - Magnetic moment ∝ rod length
  - 2× length ≈ 2× field strength
  - Better whole-house coverage
  - Can always downgrade if 100mm sufficient

### Magnet Wire
- **Spec:** 30 AWG enameled copper wire
- **Length needed:** ~50 feet for 200 turns
- **Source:** Amazon (Remington Industries)
- **Alternative:** 32 AWG (thinner = easier winding, but more fragile)
- **Winding:**
  - 200 turns on 200mm rod = ~50mm coil length
  - Wind tightly, side-by-side (no overlap)
  - Scrape enamel off ends for electrical contact

### Transistor Driver
- **Part:** 2N2222 NPN (already owned)
- **Function:** Switches antenna coil current at 60 kHz
- **Circuit:** ESP32 GPIO → base resistor → 2N2222 → coil → 5V

## FCC Part 15 Compliance

### Regulatory Limits
- **Part 15.209** applies (intentional radiator <490 kHz)
- **Field strength limit:** 2400/F(kHz) μV/m @ 300m
- **At 60 kHz:** 40 μV/m @ 300m
- **Measurement:** Calibrated loop antenna, quasi-peak detector

### Compliance Strategy

**Conservative approach:**
1. **Low power by design:** Ferrite rod antenna is inherently inefficient (electrically tiny at 60 kHz wavelength = 5 km)
2. **Building attenuation helps:** Indoor signal is contained by walls/structure
3. **Reference point:** Instructables design achieved 30+ foot range, likely well under limits
4. **Verification:** Test with WWVB receivers at increasing distances; if clocks sync at 50+ feet, probably compliant

**No formal measurement planned because:**
- Don't have calibrated field strength meter (~$10k+ equipment)
- Conservative design (ferrite rod, low current) stays well below limits
- Building contains signal (neighbors unlikely to detect)
- Continuous operation is legal under Part 15 (no duty cycle restriction)

**Risk mitigation:**
- Start with low drive current, increase only if needed for coverage
- Monitor for RFI complaints (unlikely at 60 kHz)
- If enforcement action occurs, can reduce power or cease operation

## Implementation Plan

### Phase 1: Code Development (During 1-Month Wait for Ferrite Rod)

**Tasks:**
1. Set up Arduino IDE with ESP32 board support
2. Implement WWVB encoder function
3. Implement NTP client with multi-server failover
4. Implement 1Hz timer interrupt synchronized to UTC second boundaries
5. Test encoder logic with known-good WWVB frames
6. Verify NTP sync accuracy with local stratum-1 servers

**Deliverables:**
- `wwvb_transmitter.ino` (complete Arduino sketch)
- WWVB encoder validated against NIST examples
- NTP sync verified with serial monitor output

### Phase 2: Antenna Construction (When Ferrite Rod Arrives)

**Tasks:**
1. Wind 200 turns of 30 AWG wire on ferrite rod
2. Mark turn count every 50 turns (tape)
3. Secure coil ends with tape
4. Scrape enamel off wire ends
5. Measure inductance (if LCR meter available)
6. Calculate resonant capacitor (optional optimization)

**Deliverables:**
- Completed ferrite rod antenna
- Inductance measurement (if possible)

### Phase 3: Circuit Assembly

**Tasks:**
1. Breadboard circuit: ESP32 → 2N2222 → coil
2. Calculate base resistor for 2N2222 (typical: 1kΩ)
3. Calculate current-limiting resistor for coil (start conservative: 100Ω)
4. Configure ESP32 LEDC PWM for 60 kHz output
5. Verify 60 kHz carrier with oscilloscope

**Deliverables:**
- Working transmitter on breadboard
- Oscilloscope verification of 60 kHz carrier
- Amplitude modulation verified (200ms/500ms/800ms pulses)

### Phase 4: Testing and Range Verification

**Tasks:**
1. Place WWVB clock near transmitter, verify sync
2. Move clock to increasing distances, test sync at each location
3. Map house coverage (which rooms can sync)
4. Identify dead zones or problem areas
5. Iterate on antenna placement (elevation, orientation)
6. If needed: increase drive current, add more turns, or add resonant cap

**Deliverables:**
- Coverage map of house
- All five clocks able to sync from at least one location
- Continuous operation verified (24+ hour test)

### Phase 5: Permanent Installation

**Tasks:**
1. Transfer circuit from breadboard to protoboard or custom PCB
2. Mount in enclosure (plastic project box)
3. Install in optimal location (central, elevated)
4. Secure antenna to prevent movement
5. Connect to permanent 5V power supply (USB wall adapter)
6. Verify all clocks sync over 1-week period

**Deliverables:**
- Production unit in enclosure
- Installed and tested
- Documentation for future maintenance

## Code Structure

### Main Arduino Sketch Outline
```cpp
// wwvb_transmitter.ino

#include <WiFi.h>
#include <time.h>

// Configuration
const char* ssid = "your-ssid";
const char* password = "your-password";
const char* ntpServer1 = "192.168.1.100";  // Primary stratum-1
const char* ntpServer2 = "192.168.1.101";  // Secondary stratum-1
const char* ntpServer3 = "pool.ntp.org";   // Internet fallback

const int OUTPUT_PIN = 18;  // GPIO for 60 kHz output
const int LED_PIN = 2;      // Built-in LED for status

// LEDC PWM configuration
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 60000;  // 60 kHz
const int PWM_RESOLUTION = 8; // 8-bit resolution

// WWVB state
uint8_t wwvbFrame[60];  // Current minute's time code
int currentSecond = 0;

// Timer
hw_timer_t * timer = NULL;

void setup() {
  Serial.begin(115200);
  
  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // NTP configuration (ESP32 built-in SNTP)
  configTime(0, 0, ntpServer1, ntpServer2, ntpServer3);
  
  // Wait for time sync
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP sync...");
    delay(1000);
  }
  Serial.println("Time synchronized");
  
  // PWM setup for 60 kHz carrier
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(OUTPUT_PIN, PWM_CHANNEL);
  
  // Timer setup (1 Hz interrupt)
  timer = timerBegin(0, 80, true);  // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);  // 1 second
  timerAlarmEnable(timer);
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Main loop does nothing - timer ISR handles transmission
  delay(1000);
  
  // Periodic status output
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.printf("%02d:%02d:%02d\n", 
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }
}

void IRAM_ATTR onTimer() {
  // Called every second
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;  // No valid time yet
  }
  
  // At second 0, encode next minute's frame
  if (timeinfo.tm_sec == 0) {
    // Advance time by 1 minute for encoding
    time_t now = mktime(&timeinfo);
    now += 60;
    struct tm *nextMinute = gmtime(&now);
    encodeWWVB(nextMinute, wwvbFrame);
  }
  
  // Transmit current second's bit
  int bit = wwvbFrame[timeinfo.tm_sec];
  transmitBit(bit);
  
  // Blink LED
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void encodeWWVB(struct tm *time, uint8_t *frame) {
  // TODO: Implement WWVB encoding
  // - Convert time to BCD
  // - Set frame marker bits
  // - Calculate parity bits
  // - Set DST warning bits
  
  // Placeholder: all zeros
  memset(frame, 0, 60);
  
  // Frame marker at second 0
  frame[0] = 2;  // 800ms marker
  
  // Minute encoding (BCD)
  int minute = time->tm_min;
  frame[1] = (minute >= 40) ? 1 : 0;
  frame[2] = (minute % 40 >= 20) ? 1 : 0;
  frame[3] = (minute % 20 >= 10) ? 1 : 0;
  // ... continue for all time/date fields
}

void transmitBit(int bit) {
  // Bit encoding:
  // 0 = 200ms low, 800ms high
  // 1 = 500ms low, 500ms high
  // 2 = 800ms marker
  
  int lowTime, highTime;
  
  switch(bit) {
    case 0:
      lowTime = 200;
      highTime = 800;
      break;
    case 1:
      lowTime = 500;
      highTime = 500;
      break;
    case 2:  // Marker
      lowTime = 800;
      highTime = 200;
      break;
    default:
      lowTime = 0;
      highTime = 1000;
  }
  
  // Low = carrier off
  ledcWrite(PWM_CHANNEL, 0);
  delayMicroseconds(lowTime * 1000);
  
  // High = carrier on (50% duty cycle)
  ledcWrite(PWM_CHANNEL, 128);
  delayMicroseconds(highTime * 1000);
}
```

### WWVB Encoder Details

**Key implementation notes:**
1. **Time is for NEXT minute:** WWVB transmits time for the following minute (during minute N, send time for N+1)
2. **UTC always:** WWVB is always UTC, never local time
3. **BCD encoding:** Most significant bit first
4. **Parity bits:** Even parity for designated groups
5. **DST bits:** Warning bit set from 00:00 UTC on day of change; flag bit changes at 00:00 UTC day after change
6. **Day of year:** Not day of month (clocks must calculate month/day from DoY + leap year)

**Reference:**
- NIST Special Publication 432 (WWVB time code format)
- Test cases from NIST website for validation

## Testing Strategy

### Unit Testing (Pre-Hardware)
1. **WWVB encoder:**
   - Test known dates/times against NIST examples
   - Verify BCD conversion: minute=30 → bits 40,20,10 encoded correctly
   - Verify parity: odd number of 1s → parity bit set
   - Verify DST logic: warning bit timing, flag bit timing
   - Edge cases: minute 59, leap year, DST transition day

2. **Time handling:**
   - Verify `gmtime()` returns UTC
   - Verify minute boundary detection (sec == 0)
   - Verify time advancement for next-minute encoding

### Integration Testing (With Hardware)
1. **PWM output:**
   - Oscilloscope verification: 60 kHz ± 100 Hz
   - Duty cycle: 50% when carrier on
   - Modulation: clean on/off transitions at bit boundaries

2. **Timing accuracy:**
   - Serial monitor: print current time every second
   - Verify no drift over 1 hour (NTP keeps accurate)
   - Verify bit timing: 200ms/500ms/800ms with oscilloscope

3. **Receiver testing:**
   - Place clock at 1 foot: should sync within 5 minutes
   - Move to 5 feet, 10 feet, 20 feet, 50 feet: measure max range
   - Test in different orientations (ferrite rod is directional)
   - Test through walls (wood, drywall, concrete if applicable)

### Acceptance Criteria
- All five clocks sync from their intended locations
- Continuous operation for 1 week without manual intervention
- NTP sync successful (check serial log)
- No RFI interference with other devices (AM radio, etc.)

## Known Issues and Future Improvements

### Known Limitations
1. **Ferrite rod is directional:** Signal strongest broadside to rod, weakest off the ends. May need to orient antenna or use multiple orientations.
2. **No field strength measurement:** Compliance assumed based on reference designs, not measured.
3. **Single transmitter:** If coverage gaps exist, no easy fix except repositioning or building second unit.

### Possible Improvements
1. **Resonant circuit:** Add capacitor in series with coil to resonate at 60 kHz (increases Q, improves efficiency)
2. **Web interface:** ESP32 can serve webpage showing current time, sync status, uptime
3. **OTA updates:** Enable over-the-air firmware updates for future fixes
4. **Multiple protocol support:** Add DCF77 (77.5 kHz), MSF (60 kHz UK format), JJY (40/60 kHz) for other clocks
5. **Automatic power control:** Measure receiver signal strength (if possible) and adjust transmit power dynamically

### Debugging Aids
1. **Serial output:** Verbose logging of NTP sync, time encoding, bit transmission
2. **LED patterns:** Different blink patterns for sync status, errors, normal operation
3. **Test mode:** Button to force transmission of known-good frame for verification

## References

### WWVB Protocol
- NIST Special Publication 432: https://www.nist.gov/pml/time-and-frequency-division/radio-stations/wwvb
- Wikipedia WWVB: https://en.wikipedia.org/wiki/WWVB

### Component Datasheets
- ESP32-WROOM-32 Datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf
- 2N2222 Transistor Datasheet: https://www.onsemi.com/pdf/datasheet/p2n2222a-d.pdf
- Fair-Rite Ferrite Material 61: https://fair-rite.com/product/61-material/

### Similar Projects
- Instructables WWVB Generator (ATtiny45): https://www.instructables.com/WWVB-radio-time-signal-generator-for-ATTINY45-or-A/
- Chronvertor (commercial, discontinued): Provided WWVB encoding reference

### FCC Regulations
- Part 15.209: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/section-15.209

## Revision History

- **v1.0** (2025-02-08): Initial design document capturing architecture decisions, component specs, and implementation plan

## User Context

### Technical Background
- EE degree
- Ham radio license (understands RF, FCC regulations)
- Principal software engineer (10M+ TPS distributed systems experience)
- Owns oscilloscope (can verify signals)
- Previously explored GPS-disciplined NTP for distributed transactions (Spanner-style TrueTime)

### Available Resources
- Multiple stratum-1 NTP servers (GPS-disciplined Raspberry Pi)
- Arduino Nano boards (for prototyping, though ESP32 selected)
- 2N2222 transistors (already owned)
- Basic electronics components (resistors, capacitors, wire)

### Communication Preferences
- Skip introductory explanations
- Focus on implementation details and tradeoffs
- Challenge assumptions, flag problems early
- Technical competence assumed
- Direct, engineering-focused communication

---

**End of Design Document v1.0**