# Testing Strategy

## Overview
Tests are standalone Arduino sketches uploaded to the ESP32 via Arduino IDE. Results displayed via Serial Monitor at 115200 baud.

## Arduino IDE Workflow

### How Arduino IDE Handles Files
- Arduino IDE opens individual `.ino` files, not directories
- When you open a `.ino` file, Arduino IDE automatically includes all `.h` and `.cpp` files in the same directory as tabs
- There is no equivalent to GCC's `-I` include path flag — all dependencies must be in the same directory as the `.ino` file

### Opening the Main Program
1. Arduino IDE: **File → Open**
2. Navigate to `wwvb_transmitter/wwvb_transmitter.ino`
3. Arduino IDE automatically shows all module files (`.h`/`.cpp`) as tabs
4. Select **ESP32 Dev Module** as the board
5. Select the correct COM port
6. Click **Upload**

### Running Unit Tests
1. Open the test `.ino` file directly in Arduino IDE
2. Unit tests are **fully self-contained** — no file copying needed
3. Upload to ESP32
4. Open Serial Monitor at **115200 baud**
5. Read PASS/FAIL results

### Running Integration Tests
1. Integration tests that depend on project modules require those `.h`/`.cpp` files to be in the same directory
2. The `hardware_validation` and `test_pwm_output` tests already have their dependencies inlined
3. The `test_ntp_client` test uses only ESP32 system libraries (WiFi, time) — no project dependencies
4. WiFi-dependent tests (`test_ntp_client`, `hardware_validation`) require a `credentials.h` file — see below

### WiFi Credentials Setup
WiFi and NTP credentials are stored in `credentials.h` files that are **gitignored**.

Each directory that needs WiFi has a `credentials_template.h`. To set up:
1. Copy `credentials_template.h` → `credentials.h` in that directory
2. Edit `credentials.h` with real WiFi SSID, password, and NTP server addresses

Directories with credential templates:
- `wwvb_transmitter/` (main program)
- `tests/test_ntp_client/` (NTP test)
- `tests/hardware_validation/` (full system test)

## Test Categories

### Unit Tests (Self-Contained)
Unit tests inline all necessary project code directly in the `.ino` file. No external files needed — just open in Arduino IDE and upload.

Each inlined section includes a "Source of truth" comment pointing to the canonical implementation in `wwvb_transmitter/` and a "Last synced" date. When the source of truth changes, the inlined test code must be updated to match.

### Integration Tests (System Libraries Only)
Integration tests use ESP32 system libraries (WiFi, NTP, PWM) and may inline project code where needed. All current integration tests are already self-contained.

### Hardware Validation (Full System)
The hardware validation test inlines the encoder and runs the complete NTP → encode → transmit cycle with diagnostics.

## Keeping Tests in Sync

When modifying code in `wwvb_transmitter/`:
1. Identify which test files inline the changed functions
2. Update the inlined code in those test files to match
3. Update the "Last synced" date in the inlined section header
4. Run the updated tests to verify

| Source Module | Test Files That Inline It |
|---|---|
| `wwvb_encoder.h/.cpp` | `test_wwvb_encoder.ino`, `hardware_validation.ino` |
| `dst_manager.h/.cpp` | `test_dst_calculation.ino` |
| `ntp_manager.h/.cpp` | *(none — test_ntp_client uses ESP32 libs directly)* |

## Test Framework
Simple assertion macros defined in each test sketch:
```cpp
#define TEST_ASSERT(name, condition) do { \
  totalTests++; \
  if (condition) { \
    passedTests++; \
    Serial.printf("  PASS: %s\n", name); \
  } else { \
    failedTests++; \
    Serial.printf("  FAIL: %s\n", name); \
  } \
} while(0)

#define TEST_ASSERT_EQUAL(name, expected, actual) \
  TEST_ASSERT(name, (expected) == (actual))
```

## Test Sketches

### 1. test_wwvb_encoder (Pure Logic — No Hardware)
**Location:** `tests/test_wwvb_encoder/test_wwvb_encoder.ino`
**Self-contained:** Yes — encoder functions inlined
**Inlines:** `wwvb_encoder.h/.cpp`

**Tests:**
- BCD conversion for all time fields (minutes, hours, day-of-year, year)
- Marker placement at correct positions (0, 9, 19, 29, 39, 49, 59)
- Reserved bits always zero (4, 10, 11, 14, 20, 21, 24, 34, 35, 44, 54)
- Next-minute encoding (time advancement)
- DST status bits for all four states
- Leap year indicator
- Edge cases: minute 59→0 rollover, hour 23→0, day 365→1, year boundary
- Known NIST reference frame validation

### 2. test_dst_calculation (Pure Logic — No Hardware)
**Location:** `tests/test_dst_calculation/test_dst_calculation.ino`
**Self-contained:** Yes — DST functions inlined
**Inlines:** `dst_manager.h/.cpp`

**Tests:**
- Day-of-week calculation (Sakamoto's algorithm)
- Nth weekday of month
- DST transition dates for 2024-2030
- isInDST for all months and transition hours
- Full DST status calculation for Pacific and Eastern timezones
- UTC-to-local day rollback edge cases
- Multi-year transition validation

### 3. test_ntp_client (Requires WiFi)
**Location:** `tests/test_ntp_client/test_ntp_client.ino`
**Self-contained:** Yes — uses ESP32 system libraries only
**Inlines:** Nothing (uses WiFi.h, time.h, esp_sntp.h)

**Tests:**
- WiFi connection to configured network
- NTP sync with primary server
- NTP sync with fallback servers
- Time accuracy: compare successive reads for monotonicity
- UTC verification (no local time offset)
- Reconnection after simulated WiFi drop

### 4. test_pwm_output (Requires Oscilloscope)
**Location:** `tests/test_pwm_output/test_pwm_output.ino`
**Self-contained:** Yes — uses ESP32 PWM APIs only
**Inlines:** Nothing (uses ledcAttach, ledcWrite — ESP32 Core 3.x API)

**Tests:**
- 60 kHz PWM output on configured GPIO
- Carrier on/off control
- Bit timing verification:
  - Binary 0: 200ms off, 800ms on
  - Binary 1: 500ms off, 500ms on
  - Marker: 800ms off, 200ms on
- Full frame transmission (60 seconds)

### 5. hardware_validation (Full System)
**Location:** `tests/hardware_validation/hardware_validation.ino`
**Self-contained:** Yes — encoder functions inlined
**Inlines:** `wwvb_encoder` functions (compact version)

**Tests:**
- Complete system integration
- NTP sync → encode → transmit cycle
- Serial output of each bit as transmitted
- LED status indication
- Continuous operation stability

## Test Output Format
```
========================================
  WWVB Encoder Test Suite
========================================

--- BCD Conversion Tests ---
  PASS: Minute 0 encodes correctly
  PASS: Minute 30 encodes correctly
  PASS: Minute 59 encodes correctly
  FAIL: Hour 23 encodes correctly

--- Marker Position Tests ---
  PASS: Position 0 is marker
  PASS: Position 9 is marker
  ...

========================================
  Results: 42/43 PASSED, 1 FAILED
========================================
```

## Test Data: NIST Reference Frames

### Reference Frame 1: 2025-01-01 00:01 UTC
Used to validate encoder against known-good output.

### Reference Frame 2: 2024-12-31 23:59 UTC  
Tests year boundary and next-minute rollover.

### Reference Frame 3: DST Spring Forward (2025-03-09)
Tests DST warning bit transition.

### Reference Frame 4: Leap Year Day 366 (2024-12-31)
Tests leap year flag and max day-of-year.

## Acceptance Criteria

### Software Tests (Must Pass Before Hardware)
- [ ] All BCD conversions correct for boundary values
- [ ] All marker positions correct
- [ ] All reserved bits are zero
- [ ] Next-minute encoding handles all rollovers
- [ ] DST bits set correctly for all four states
- [ ] At least one NIST reference frame matches exactly

### Hardware Tests (With Oscilloscope)
- [ ] 60 kHz carrier within ±100 Hz
- [ ] 50% duty cycle when carrier on
- [ ] Bit timing within ±5ms of specification
- [ ] Clean on/off transitions (no ringing >1 cycle)

### Integration Tests (With WWVB Clock)
- [ ] Clock syncs within 5 minutes at 1 foot
- [ ] Clock syncs within 10 minutes at 10 feet
- [ ] All five clocks sync from installed locations
- [ ] 24-hour continuous operation without errors
