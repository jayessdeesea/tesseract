# DST Calculation Specification

## Overview

The WWVB transmitter automatically calculates DST status bits from UTC time and the configured US timezone. No manual intervention is needed for DST transitions.

## US DST Rules

Per the **Energy Policy Act of 2005** (effective 2007):
- **Begins:** 2nd Sunday in March at 2:00 AM local standard time
- **Ends:** 1st Sunday in November at 2:00 AM local daylight time

### Supported Timezones

| Timezone | Standard Offset | Daylight Offset | Enum Value |
|----------|----------------|-----------------|------------|
| US Eastern | UTC-5 (EST) | UTC-4 (EDT) | `TZ_US_EASTERN` |
| US Central | UTC-6 (CST) | UTC-5 (CDT) | `TZ_US_CENTRAL` |
| US Mountain | UTC-7 (MST) | UTC-6 (MDT) | `TZ_US_MOUNTAIN` |
| US Pacific | UTC-8 (PST) | UTC-7 (PDT) | `TZ_US_PACIFIC` |

**Default:** US Pacific (Seattle, WA)

### Exclusions

Arizona (no DST) and Hawaii (no DST) are not currently supported. These locations would need `DST_STANDARD` hardcoded.

## WWVB DST Status Bits

The WWVB frame encodes DST status in bits 57-58:

| Bit 57 | Bit 58 | Status | Constant | Meaning |
|--------|--------|--------|----------|---------|
| 0 | 0 | Standard Time | `DST_STANDARD` (0x00) | Standard time in effect |
| 1 | 0 | DST Begins | `DST_BEGINS` (0x02) | DST transition day (spring forward) |
| 1 | 1 | DST In Effect | `DST_IN_EFFECT` (0x03) | Daylight saving time active |
| 0 | 1 | DST Ends | `DST_ENDS` (0x01) | DST transition day (fall back) |

### Transition Day Behavior

On the **spring forward** day (BEGINS):
- The entire local calendar day reports `DST_BEGINS`
- Clocks spring forward at 2:00 AM → 3:00 AM local

On the **fall back** day (ENDS):
- The entire local calendar day reports `DST_ENDS`
- Clocks fall back at 2:00 AM → 1:00 AM local

## Algorithm

### Step 1: UTC to Local Standard Time

Convert UTC time to local time using the **standard** timezone offset (always negative for US):

```
localHour = utcHour + timezoneOffset
```

If `localHour < 0`, roll back to the previous day (and potentially previous month/year).

### Step 2: Determine Transition Dates

Calculate the DST transition dates for the current local year:
- **Start:** `nthWeekdayOfMonth(year, March, Sunday, 2nd)`
- **End:** `nthWeekdayOfMonth(year, November, Sunday, 1st)`

Day-of-week calculation uses **Tomohiko Sakamoto's algorithm** — pure arithmetic, no lookup tables beyond a 12-element month offset array.

### Step 3: Check Transition Day

If the local date is the spring forward day → return `DST_BEGINS`
If the local date is the fall back day → return `DST_ENDS`

### Step 4: Determine Current State

If neither transition day, check if the local date/time falls within the DST period:
- **Before March transition day:** Standard
- **After March transition day, before November transition day:** DST
- **After November transition day:** Standard

## Known DST Transition Dates (US)

| Year | Spring Forward (2nd Sun Mar) | Fall Back (1st Sun Nov) |
|------|------------------------------|-------------------------|
| 2024 | March 10 | November 3 |
| 2025 | March 9 | November 2 |
| 2026 | March 8 | November 1 |
| 2027 | March 14 | November 7 |
| 2028 | March 12 | November 5 |
| 2029 | March 11 | November 4 |
| 2030 | March 10 | November 3 |

## UTC-to-Local Edge Cases

For westward timezones (negative UTC offset), early UTC hours map to the **previous local day**:

| UTC Time | Pacific Local | Local Date |
|----------|--------------|------------|
| Mar 9 05:00 UTC | Mar 8 21:00 PST | Previous day (not transition) |
| Mar 9 08:00 UTC | Mar 9 00:00 PST | Transition day ✓ |
| Mar 10 06:00 UTC | Mar 9 22:00 PST | Still transition day |
| Nov 2 05:00 UTC | Nov 1 21:00 PST | Previous day (still DST) |
| Nov 2 08:00 UTC | Nov 2 00:00 PST | Transition day ✓ |
| Jan 1 03:00 UTC | Dec 31 19:00 PST | Previous year |

## Implementation Files

- **Header:** `wwvb_transmitter/dst_manager.h`
- **Implementation:** `wwvb_transmitter/dst_manager.cpp`
- **Configuration:** `wwvb_transmitter/config.h` (`DEFAULT_TIMEZONE`)
- **Tests:** `tests/test_dst_calculation/test_dst_calculation.ino`

## Test Coverage

The DST test suite validates:
1. **Day-of-week algorithm** — known dates verified against calendar
2. **Nth weekday calculation** — 2nd Sunday, 1st Sunday for multiple years
3. **Transition date accuracy** — 2024-2030 verified against timeanddate.com
4. **isInDST logic** — month boundaries, transition day hours, all 12 months
5. **Full calculateDSTStatus** — Pacific and Eastern timezones
6. **UTC-to-local rollback** — previous day, month, and year boundary cases
7. **Multi-year validation** — 7 years of transition day/before/after tests
8. **Edge cases** — midnight UTC, year boundaries, leap years
