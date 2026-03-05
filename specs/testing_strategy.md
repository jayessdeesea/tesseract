# Testing Strategy

## Overview
Tests are standalone Arduino sketches uploaded to the ESP32 via Arduino IDE. Results displayed via Serial Monitor at 115200 baud.

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

### 1. test_wwvb_encoder (Pure Logic - No Hardware)
**Location:** `tests/test_wwvb_encoder/test_wwvb_encoder.ino`

**Tests:**
- BCD conversion for all time fields (minutes, hours, day-of-year, year)
- Marker placement at correct positions (0, 9, 19, 29, 39, 49, 59)
- Reserved bits always zero (4, 10, 11, 14, 20, 21, 24, 34, 35, 44, 54)
- Next-minute encoding (time advancement)
- DST status bits for all four states
- Leap year indicator
- Edge cases: minute 59→0 rollover, hour 23→0, day 365→1, year boundary
- Known NIST reference frame validation

### 2. test_ntp_client (Requires WiFi)
**Location:** `tests/test_ntp_client/test_ntp_client.ino`

**Tests:**
- WiFi connection to configured network
- NTP sync with primary server
- NTP sync with fallback servers
- Time accuracy: compare successive reads for monotonicity
- UTC verification (no local time offset)
- Reconnection after simulated WiFi drop

### 3. test_pwm_output (Requires Oscilloscope)
**Location:** `tests/test_pwm_output/test_pwm_output.ino`

**Tests:**
- 60 kHz PWM output on configured GPIO
- Carrier on/off control
- Bit timing verification:
  - Binary 0: 200ms off, 800ms on
  - Binary 1: 500ms off, 500ms on
  - Marker: 800ms off, 200ms on
- Full frame transmission (60 seconds)

### 4. hardware_validation (Full System)
**Location:** `tests/hardware_validation/hardware_validation.ino`

**Tests:**
- Complete system integration
- NTP sync → encode → transmit cycle
- Serial output of each bit as transmitted
- LED status indication
- Continuous operation stability

## Test Execution Workflow

### Running a Test
1. Open test `.ino` in Arduino IDE
2. Select "ESP32 Dev Module" as board
3. Select correct COM port
4. Upload sketch
5. Open Serial Monitor (115200 baud)
6. Read PASS/FAIL results

### Test Output Format
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

### Reference Frame 1: 2025-01-01 00:00 UTC
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
