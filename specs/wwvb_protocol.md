# WWVB Protocol Specification

## Overview
WWVB is the 60 kHz time signal broadcast from Fort Collins, CO by NIST. This document specifies the amplitude-modulated (AM) legacy time code format used by consumer atomic clocks.

## Carrier Signal
- **Frequency:** 60,000 Hz (60 kHz)
- **Wavelength:** 5 km
- **Modulation:** Amplitude modulation (on/off keying for our implementation)
- **Frame duration:** 60 seconds (1 bit per second)

## Bit Encoding (Amplitude Modulation)

At the start of each second, the carrier power is reduced. The duration of the reduced-power period encodes the bit:

| Bit Type | Reduced Power Duration | Full Power Duration | Total |
|----------|----------------------|--------------------|----|
| Binary 0 | 200 ms | 800 ms | 1000 ms |
| Binary 1 | 500 ms | 500 ms | 1000 ms |
| Marker   | 800 ms | 200 ms | 1000 ms |

**Implementation note:** Our transmitter uses on/off keying (carrier completely off during reduced-power period). Consumer clocks detect the amplitude transition, not the absolute power level.

## Frame Format (60 bits)

### Bit Position Map

```
Sec  Content                    Encoding
---  -------                    --------
 0   Frame reference marker     MARKER (P0)
 1   Minutes 40-weight          BCD
 2   Minutes 20-weight          BCD
 3   Minutes 10-weight          BCD
 4   Reserved (always 0)        ZERO
 5   Minutes 8-weight           BCD
 6   Minutes 4-weight           BCD
 7   Minutes 2-weight           BCD
 8   Minutes 1-weight           BCD
 9   Position marker #1         MARKER (P1)
10   Reserved (always 0)        ZERO
11   Reserved (always 0)        ZERO
12   Hours 20-weight            BCD
13   Hours 10-weight            BCD
14   Reserved (always 0)        ZERO
15   Hours 8-weight             BCD
16   Hours 4-weight             BCD
17   Hours 2-weight             BCD
18   Hours 1-weight             BCD
19   Position marker #2         MARKER (P2)
20   Reserved (always 0)        ZERO
21   Reserved (always 0)        ZERO
22   Day of year 200-weight     BCD
23   Day of year 100-weight     BCD
24   Reserved (always 0)        ZERO
25   Day of year 80-weight      BCD
26   Day of year 40-weight      BCD
27   Day of year 20-weight      BCD
28   Day of year 10-weight      BCD
29   Position marker #3         MARKER (P3)
30   Day of year 8-weight       BCD
31   Day of year 4-weight       BCD
32   Day of year 2-weight       BCD
33   Day of year 1-weight       BCD
34   Reserved (always 0)        ZERO
35   Reserved (always 0)        ZERO
36   UT1 sign positive          DATA
37   UT1 sign negative          DATA
38   UT1 sign positive          DATA
39   Position marker #4         MARKER (P4)
40   UT1 correction 0.8         BCD
41   UT1 correction 0.4         BCD
42   UT1 correction 0.2         BCD
43   UT1 correction 0.1         BCD
44   Reserved (always 0)        ZERO
45   Year 80-weight             BCD
46   Year 40-weight             BCD
47   Year 20-weight             BCD
48   Year 10-weight             BCD
49   Position marker #5         MARKER (P5)
50   Year 8-weight              BCD
51   Year 4-weight              BCD
52   Year 2-weight              BCD
53   Year 1-weight              BCD
54   Reserved (always 0)        ZERO
55   Leap year indicator        DATA (1 = leap year)
56   Leap second warning        DATA (1 = leap second this month)
57   DST status bit 1           DATA
58   DST status bit 2           DATA
59   Frame reference marker     MARKER (P0)
```

### Position Markers
Markers appear at seconds: 0, 9, 19, 29, 39, 49, 59
- Second 0 and 59: Frame reference markers (two consecutive markers = frame sync)
- Others: Position identifiers within frame

### Reserved Bits
Always transmitted as binary 0 (200ms reduced power):
Seconds: 4, 10, 11, 14, 20, 21, 24, 34, 35, 44, 54

## Time Encoding Details

### Critical Rule: Next-Minute Encoding
**WWVB transmits the time for the UPCOMING minute.** During minute N, the frame encodes time for minute N+1. This is because the receiver needs the full 60 seconds to decode the frame, and the decoded time should be correct when the frame ends.

### Time Base
- **Always UTC** - never local time
- Clocks handle UTC→local conversion internally using DST bits
- Year is 2-digit (00-99), representing offset from 2000

### BCD Encoding
All numeric values use Binary Coded Decimal, most significant weight first:
- **Minutes (0-59):** Weights 40, 20, 10, 8, 4, 2, 1
- **Hours (0-23):** Weights 20, 10, 8, 4, 2, 1
- **Day of year (1-366):** Weights 200, 100, 80, 40, 20, 10, 8, 4, 2, 1
- **Year (0-99):** Weights 80, 40, 20, 10, 8, 4, 2, 1

### DST Status Bits (seconds 57-58)

| Bit 57 | Bit 58 | Meaning |
|--------|--------|---------|
| 0      | 0      | Standard Time in effect |
| 1      | 0      | DST begins today (at 02:00 local) |
| 1      | 1      | DST in effect |
| 0      | 1      | DST ends today (at 02:00 local) |

**DST transition timing:**
- Warning: Set bits at 00:00 UTC on the day of change
- After change takes effect: Update to new steady-state

### Leap Year Indicator (second 55)
- Set to 1 if the current year is a leap year
- Helps clocks calculate month/day from day-of-year

### Leap Second Warning (second 56)
- Set to 1 if a leap second will occur at the end of the current month
- Our implementation: always 0 (leap seconds are extremely rare and being phased out)

### UT1 Correction (seconds 36-43)
- Sign: seconds 36, 38 = positive sign; second 37 = negative sign
- Magnitude: BCD 0.0-0.9 seconds (weights 0.8, 0.4, 0.2, 0.1)
- Our implementation: set to +0.0 (not meaningful for consumer clocks)

## Encoding Example

### Example: 2025-02-08 15:30 UTC, Standard Time, Not Leap Year

**Next minute time (what we encode): 15:31 UTC, Day 39 of 2025**

```
Minutes = 31: 40=0, 20=1, 10=1, 8=0, 4=0, 2=1, 1=1
Hours   = 15: 20=0, 10=1, 8=1, 4=1, 2=1, 1=1  (wait: 8+4+2+1=15, 10=1 means 10+5=15)
                    Actually: 20=0, 10=1, 8=0, 4=1, 2=0, 1=1  (10+4+1=15)
Day     = 39: 200=0, 100=0, 80=0, 40=0, 20=1, 10=1, 8=1, 4=0, 2=0, 1=1
               (20+10+8+1=39)
Year    = 25: 80=0, 40=0, 20=1, 10=0, 8=0, 4=1, 2=0, 1=1  (20+4+1=25)
```

Frame bits:
```
M 0 1 1 0  0 0 1 1 M   (sec 0-9: marker, min=31)
0 0 0 1 0  0 1 0 1 M   (sec 10-19: hours=15)
0 0 0 0 0  0 1 1 1 M   (sec 20-29: day high=039)
0 0 1 0 0  1 0 0 1 M   (sec 30-39: day low, UT1 sign)
0 0 0 0 0  0 0 1 0 M   (sec 40-49: UT1=0, year high=25)
0 1 0 1 0  0 0 0 0 M   (sec 50-59: year low, LY=0, LS=0, DST=00)
```
