# System Architecture

## Selected Architecture: ESP32 + NTP + Ferrite Rod Antenna

### Core Components
1. **ESP32 DevKit** (ESP32-WROOM-32D, HiLetgo B08D5ZD528)
   - Dual-core 240 MHz, WiFi built-in
   - Hardware PWM (LEDC peripheral) for 60 kHz generation
   - ~$6 cost, Arduino IDE compatible

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

4. **Darlington Driver**
   - ULN2003AN Darlington transistor array (DIP-16)
   - Internal input resistors and flyback diodes
   - Drives antenna coil at 60 kHz
   - 500mA capacity, 3.3V logic compatible

## Why This Architecture Wins

### Time Accuracy
- NTP sync to local GPS-disciplined servers: sub-millisecond accuracy
- No drift between syncs (1-6 hour intervals)
- No manual intervention after WiFi setup
- Automatic handling of DST, leap seconds

### Timing Precision
- ESP32 hardware timer: sub-microsecond jitter
- FreeRTOS guarantees for interrupt priority
- LEDC PWM peripheral: hardware-generated 60 kHz, independent of CPU

### Simplicity
- Single device, ~300 lines of Arduino C code
- No external RTC, GPS module, or complex RF
- Firmware updates over WiFi (OTA possible)

### Reliability
- Embedded system, no moving parts
- Watchdog timer auto-recovery
- No SD card corruption risk
- Power failure recovery: auto-reconnect and re-sync

### Cost
- ESP32: $6, Ferrite rod: $12, Magnet wire: $9, ULN2003AN/resistors: $2
- **Total: ~$29**

## Rejected Approaches

### Commercial Solution (Chronvertor)
- Discontinued, not available
- Battery-backed RTC (DS3231) drifts ~5 seconds/month
- Solves wrong problem (standalone time source vs. signal reception)

### Raspberry Pi + NTPd + Separate Oscillator
- Linux scheduler jitter 100μs-10ms (not real-time)
- GPIO bit-banging has unpredictable latency
- Cost: $60-80 vs $6, Power: 3-5W vs 0.5W
- SD card corruption, OS update complexity

### Multiple Transmitters (One Per Room)
- Beat frequency interference from crystal tolerance differences
- Phase interference creates nulls throughout house
- FCC: combined field strength matters, not per-device
- One centrally-located transmitter is simpler and more effective

## Software Architecture

### Module Decomposition
```
wwvb_transmitter.ino    → Main sketch (quantum state machine, coordination)
├── config.h            → WiFi credentials, NTP servers, pin assignments
├── wwvb_encoder.h/cpp  → WWVB time code encoding (pure logic, no hardware)
├── ntp_manager.h/cpp   → NTP sync with multi-server failover
├── dst_manager.h/cpp   → Automatic DST calculation from UTC + timezone
├── display_manager.h/cpp → 4× HT16K33 7-segment displays (local time)
├── status_leds.h/cpp   → 3 status LEDs (NTP/WiFi/TX)
└── debug_utils.h       → Serial output helpers, LED status patterns
```

### Data Flow
```
NTP Server → WiFi → ESP32 time sync → UTC time
UTC time → WWVB encoder → 60-bit frame buffer
UTC time → DST manager → DST status bits
UTC time → Display manager → Local time on 7-segment displays
Frame buffer → Quantum scheduler → Carrier OFF/ON at precise timing
LEDC PWM → ULN2003AN driver → Ferrite rod antenna → RF field
```

### Timing Architecture — Quantum State Machine

The main loop runs on a **100ms quantum grid** aligned to UTC second boundaries.
No blocking delays — all timing is achieved through quantum scheduling.

**Quantum cycle (execute-first pattern):**
1. **Sleep** for remainder of 100ms quantum
2. **Execute** the action calculated in the previous quantum
3. **Read** current state (time, NTP status)
4. **Calculate** the action for the next quantum

**WWVB bit timing maps perfectly to 100ms quanta (10 per second):**
```
Quantum 0: Carrier OFF (aligned to UTC second boundary)
Quantum 1: (carrier still off)
Quantum 2: Carrier ON if bit=0 (200ms low complete)
Quantum 3: Display update (non-critical timing slot)
Quantum 4: (carrier on for bit=0, still off for bit=1)
Quantum 5: Carrier ON if bit=1 (500ms low complete)
Quantum 6-7: (carrier on, maintenance at Q7 of second 30)
Quantum 8: Carrier ON if marker (800ms low complete)
Quantum 9: Schedule carrier OFF for next second boundary
```

**Benefits over blocking delay() approach:**
- **Smooth display updates** — always at quantum 3 (300ms), independent of bit type
- **Predictable timing** — all actions aligned to 100ms grid
- **No jitter** — display and transmission timing are decoupled
- **Non-blocking** — status LEDs update every quantum (10 Hz)
- **Catch-up logic** — if a quantum overruns, logs warning and recovers
