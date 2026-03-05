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

4. **Transistor Driver**
   - 2N2222 NPN transistor
   - Base resistor from ESP32 GPIO
   - Drives antenna coil at 60 kHz

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
- ESP32: $6, Ferrite rod: $12, Magnet wire: $9, Transistor/resistors: $2
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
wwvb_transmitter.ino  → Main sketch (setup, loop, coordination)
├── config.h          → WiFi credentials, NTP servers, pin assignments
├── wwvb_encoder.h/cpp→ WWVB time code encoding (pure logic, no hardware)
├── ntp_manager.h/cpp → NTP sync with multi-server failover
└── debug_utils.h     → Serial output helpers, LED status patterns
```

### Data Flow
```
NTP Server → WiFi → ESP32 time sync → UTC time
UTC time → WWVB encoder → 60-bit frame buffer
Frame buffer → Timer ISR (1Hz) → Bit selector
Bit selector → LEDC PWM control → 60 kHz carrier modulation
Carrier → 2N2222 driver → Ferrite rod antenna → RF field
```

### Timing Architecture
- **1 Hz hardware timer interrupt**: Fires at UTC second boundaries
- **ISR sets bit timing**: Configures carrier off/on durations per bit type
- **LEDC hardware PWM**: Generates 60 kHz independently of CPU
- **Main loop**: Handles NTP resync, serial output, watchdog
