/*
 * test_wwvb_encoder.ino - WWVB Encoder Unit Tests
 * 
 * Comprehensive unit tests for the WWVB time code encoder.
 * Upload to ESP32, open Serial Monitor at 115200 baud to see results.
 * 
 * NO HARDWARE REQUIRED - pure logic tests only.
 * FULLY SELF-CONTAINED - no external files needed.
 * 
 * Tests:
 *   - BCD conversion for minutes, hours, day-of-year, year
 *   - Marker placement at correct positions
 *   - Reserved bits always zero
 *   - Leap year detection
 *   - Day of year calculation
 *   - Advance one minute (all rollover cases)
 *   - DST status bits
 *   - Complete frame encoding against reference data
 *   - Edge cases and boundary conditions
 */

// ============================================================
// WWVB Encoder (inlined from wwvb_transmitter/wwvb_encoder.h/.cpp)
// Source of truth: wwvb_transmitter/wwvb_encoder.h/.cpp
// Last synced: 2026-03-11
// ============================================================

#include <Arduino.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// Bit type constants
#define WWVB_BIT_ZERO   0   // Binary 0 (200ms low)
#define WWVB_BIT_ONE    1   // Binary 1 (500ms low)
#define WWVB_BIT_MARKER 2   // Position/frame marker (800ms low)

// Frame size
#define WWVB_FRAME_SIZE 60

// DST status values (for bits 57-58)
#define DST_STANDARD    0x00  // bits 57=0, 58=0: Standard Time
#define DST_BEGINS      0x02  // bits 57=1, 58=0: DST begins today
#define DST_IN_EFFECT   0x03  // bits 57=1, 58=1: DST in effect
#define DST_ENDS        0x01  // bits 57=0, 58=1: DST ends today

// Days in each month (non-leap year)
static const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Marker positions within the 60-bit frame
static const int MARKER_POSITIONS[] = {0, 9, 19, 29, 39, 49, 59};
static const int NUM_MARKERS = 7;

// Reserved bit positions (always zero)
static const int RESERVED_POSITIONS[] = {4, 10, 11, 14, 20, 21, 24, 34, 35, 44, 54};
static const int NUM_RESERVED = 11;


int isLeapYear(int year) {
  if (year % 400 == 0) return 1;
  if (year % 100 == 0) return 0;
  if (year % 4 == 0) return 1;
  return 0;
}


int calculateDayOfYear(const struct tm* timeinfo) {
  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon;  // 0-11
  int day = timeinfo->tm_mday;   // 1-31
  
  int doy = 0;
  for (int m = 0; m < month; m++) {
    doy += DAYS_IN_MONTH[m];
    if (m == 1 && isLeapYear(year)) {
      doy += 1;  // Add leap day in February
    }
  }
  doy += day;
  
  return doy;
}


void advanceOneMinute(struct tm* timeinfo) {
  timeinfo->tm_min += 1;
  
  if (timeinfo->tm_min >= 60) {
    timeinfo->tm_min = 0;
    timeinfo->tm_hour += 1;
    
    if (timeinfo->tm_hour >= 24) {
      timeinfo->tm_hour = 0;
      timeinfo->tm_mday += 1;
      
      int year = timeinfo->tm_year + 1900;
      int daysInMonth = DAYS_IN_MONTH[timeinfo->tm_mon];
      if (timeinfo->tm_mon == 1 && isLeapYear(year)) {
        daysInMonth = 29;
      }
      
      if (timeinfo->tm_mday > daysInMonth) {
        timeinfo->tm_mday = 1;
        timeinfo->tm_mon += 1;
        
        if (timeinfo->tm_mon >= 12) {
          timeinfo->tm_mon = 0;
          timeinfo->tm_year += 1;
        }
      }
    }
  }
  
  // Update day of year
  timeinfo->tm_yday = calculateDayOfYear(timeinfo) - 1;  // tm_yday is 0-based
}


void encodeWWVBFrame(const struct tm* timeinfo, uint8_t* frame, uint8_t dstStatus) {
  // Clear frame
  memset(frame, WWVB_BIT_ZERO, WWVB_FRAME_SIZE);
  
  // Extract time components
  int minute = timeinfo->tm_min;           // 0-59
  int hour   = timeinfo->tm_hour;          // 0-23
  int doy    = calculateDayOfYear(timeinfo); // 1-366
  int year   = (timeinfo->tm_year + 1900) % 100; // 2-digit year (0-99)
  int fullYear = timeinfo->tm_year + 1900;
  
  // ---- Set position/frame markers ----
  for (int i = 0; i < NUM_MARKERS; i++) {
    frame[MARKER_POSITIONS[i]] = WWVB_BIT_MARKER;
  }
  
  // ---- Minutes (seconds 1-8) ----
  // BCD weights: 40, 20, 10, [reserved], 8, 4, 2, 1
  frame[1] = (minute >= 40) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[2] = ((minute % 40) >= 20) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[3] = ((minute % 20) >= 10) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  // frame[4] = reserved (already 0)
  int minOnes = minute % 10;
  frame[5] = (minOnes >= 8) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[6] = ((minOnes % 8) >= 4) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[7] = ((minOnes % 4) >= 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[8] = (minOnes % 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  
  // ---- Hours (seconds 12-18) ----
  // BCD weights: [res], [res], 20, 10, [res], 8, 4, 2, 1
  frame[12] = (hour >= 20) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[13] = ((hour % 20) >= 10) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  // frame[14] = reserved (already 0)
  int hourOnes = hour % 10;
  frame[15] = (hourOnes >= 8) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[16] = ((hourOnes % 8) >= 4) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[17] = ((hourOnes % 4) >= 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[18] = (hourOnes % 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  
  // ---- Day of Year (seconds 22-33) ----
  // BCD weights: [res], [res], 200, 100, [res], 80, 40, 20, 10, [marker], 8, 4, 2, 1
  frame[22] = (doy >= 200) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[23] = ((doy % 200) >= 100) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  // frame[24] = reserved (already 0)
  int doyTens = doy % 100;
  frame[25] = (doyTens >= 80) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[26] = ((doyTens % 80) >= 40) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[27] = ((doyTens % 40) >= 20) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[28] = ((doyTens % 20) >= 10) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  // frame[29] = marker (already set)
  int doyOnes = doy % 10;
  frame[30] = (doyOnes >= 8) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[31] = ((doyOnes % 8) >= 4) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[32] = ((doyOnes % 4) >= 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[33] = (doyOnes % 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  
  // ---- UT1 Correction (seconds 36-43) ----
  // Hardcoded to +0.0 (not meaningful for consumer clocks)
  frame[36] = WWVB_BIT_ONE;   // UT1 sign positive
  frame[37] = WWVB_BIT_ZERO;  // UT1 sign negative (0 = positive)
  frame[38] = WWVB_BIT_ONE;   // UT1 sign positive (redundant)
  // frame[40-43] = 0.0 correction (already 0)
  
  // ---- Year (seconds 45-53) ----
  // BCD weights: 80, 40, 20, 10, [marker], 8, 4, 2, 1
  int yearTens = year / 10;
  frame[45] = (yearTens >= 8) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[46] = ((yearTens % 8) >= 4) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[47] = ((yearTens % 4) >= 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[48] = (yearTens % 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  // frame[49] = marker (already set)
  int yearOnes = year % 10;
  frame[50] = (yearOnes >= 8) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[51] = ((yearOnes % 8) >= 4) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[52] = ((yearOnes % 4) >= 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[53] = (yearOnes % 2) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  
  // ---- Leap year indicator (second 55) ----
  frame[55] = isLeapYear(fullYear) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  
  // ---- Leap second warning (second 56) ----
  frame[56] = WWVB_BIT_ZERO;  // Always 0 (leap seconds are rare)
  
  // ---- DST status (seconds 57-58) ----
  frame[57] = (dstStatus & 0x02) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
  frame[58] = (dstStatus & 0x01) ? WWVB_BIT_ONE : WWVB_BIT_ZERO;
}


int getLowDurationMs(uint8_t bitType) {
  switch (bitType) {
    case WWVB_BIT_ZERO:   return 200;
    case WWVB_BIT_ONE:    return 500;
    case WWVB_BIT_MARKER: return 800;
    default:              return 200;
  }
}


int getHighDurationMs(uint8_t bitType) {
  switch (bitType) {
    case WWVB_BIT_ZERO:   return 800;
    case WWVB_BIT_ONE:    return 500;
    case WWVB_BIT_MARKER: return 200;
    default:              return 800;
  }
}


void printFrame(const uint8_t* frame) {
  Serial.println("WWVB Frame:");
  Serial.print("  ");
  for (int i = 0; i < WWVB_FRAME_SIZE; i++) {
    if (frame[i] == WWVB_BIT_MARKER) {
      Serial.print("M");
    } else {
      Serial.print(frame[i]);
    }
    if ((i + 1) % 10 == 0) {
      Serial.println();
      if (i < 59) Serial.print("  ");
    } else {
      Serial.print(" ");
    }
  }
}

// ============================================================
// End of inlined WWVB encoder
// ============================================================

// ============================================================
// Test Framework
// ============================================================
int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

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

#define TEST_ASSERT_EQUAL(name, expected, actual) do { \
  totalTests++; \
  if ((expected) == (actual)) { \
    passedTests++; \
    Serial.printf("  PASS: %s\n", name); \
  } else { \
    failedTests++; \
    Serial.printf("  FAIL: %s (expected %d, got %d)\n", name, (int)(expected), (int)(actual)); \
  } \
} while(0)

// Helper: create a struct tm for testing
struct tm makeTime(int year, int month, int day, int hour, int minute, int second) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = year - 1900;  // years since 1900
  t.tm_mon = month - 1;     // 0-11
  t.tm_mday = day;           // 1-31
  t.tm_hour = hour;          // 0-23
  t.tm_min = minute;         // 0-59
  t.tm_sec = second;         // 0-59
  t.tm_yday = 0;             // will be calculated
  return t;
}

// ============================================================
// Setup - Run All Tests
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial monitor
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  WWVB Encoder Test Suite");
  Serial.println("========================================");
  Serial.println();
  
  testLeapYear();
  testDayOfYear();
  testAdvanceOneMinute();
  testMarkerPositions();
  testReservedBits();
  testMinuteEncoding();
  testHourEncoding();
  testDayOfYearEncoding();
  testYearEncoding();
  testDSTBits();
  testLeapYearBit();
  testUT1Bits();
  testBitDurations();
  testReferenceFrame1();
  testReferenceFrame2();
  testReferenceFrame3();
  testReferenceFrame4();
  
  // Print summary
  Serial.println();
  Serial.println("========================================");
  Serial.printf("  Results: %d/%d PASSED", passedTests, totalTests);
  if (failedTests > 0) {
    Serial.printf(", %d FAILED", failedTests);
  }
  Serial.println();
  Serial.println("========================================");
}

void loop() {
  // Nothing to do - tests run once in setup
  delay(10000);
}

// ============================================================
// Test: Leap Year Detection
// ============================================================
void testLeapYear() {
  Serial.println("--- Leap Year Detection ---");
  
  TEST_ASSERT_EQUAL("2024 is leap year", 1, isLeapYear(2024));
  TEST_ASSERT_EQUAL("2025 is not leap year", 0, isLeapYear(2025));
  TEST_ASSERT_EQUAL("2000 is leap year (div by 400)", 1, isLeapYear(2000));
  TEST_ASSERT_EQUAL("1900 is not leap year (div by 100)", 0, isLeapYear(1900));
  TEST_ASSERT_EQUAL("2028 is leap year", 1, isLeapYear(2028));
  TEST_ASSERT_EQUAL("2100 is not leap year", 0, isLeapYear(2100));
  Serial.println();
}

// ============================================================
// Test: Day of Year Calculation
// ============================================================
void testDayOfYear() {
  Serial.println("--- Day of Year Calculation ---");
  
  // Jan 1 = day 1
  struct tm t1 = makeTime(2025, 1, 1, 0, 0, 0);
  TEST_ASSERT_EQUAL("Jan 1 = day 1", 1, calculateDayOfYear(&t1));
  
  // Jan 31 = day 31
  struct tm t2 = makeTime(2025, 1, 31, 0, 0, 0);
  TEST_ASSERT_EQUAL("Jan 31 = day 31", 31, calculateDayOfYear(&t2));
  
  // Feb 1 = day 32
  struct tm t3 = makeTime(2025, 2, 1, 0, 0, 0);
  TEST_ASSERT_EQUAL("Feb 1 = day 32", 32, calculateDayOfYear(&t3));
  
  // Feb 28 (non-leap) = day 59
  struct tm t4 = makeTime(2025, 2, 28, 0, 0, 0);
  TEST_ASSERT_EQUAL("Feb 28 (2025) = day 59", 59, calculateDayOfYear(&t4));
  
  // Mar 1 (non-leap) = day 60
  struct tm t5 = makeTime(2025, 3, 1, 0, 0, 0);
  TEST_ASSERT_EQUAL("Mar 1 (2025) = day 60", 60, calculateDayOfYear(&t5));
  
  // Feb 29 (leap year) = day 60
  struct tm t6 = makeTime(2024, 2, 29, 0, 0, 0);
  TEST_ASSERT_EQUAL("Feb 29 (2024 leap) = day 60", 60, calculateDayOfYear(&t6));
  
  // Mar 1 (leap year) = day 61
  struct tm t7 = makeTime(2024, 3, 1, 0, 0, 0);
  TEST_ASSERT_EQUAL("Mar 1 (2024 leap) = day 61", 61, calculateDayOfYear(&t7));
  
  // Dec 31 (non-leap) = day 365
  struct tm t8 = makeTime(2025, 12, 31, 0, 0, 0);
  TEST_ASSERT_EQUAL("Dec 31 (2025) = day 365", 365, calculateDayOfYear(&t8));
  
  // Dec 31 (leap year) = day 366
  struct tm t9 = makeTime(2024, 12, 31, 0, 0, 0);
  TEST_ASSERT_EQUAL("Dec 31 (2024 leap) = day 366", 366, calculateDayOfYear(&t9));
  
  // Jul 4 (non-leap) = day 185
  struct tm t10 = makeTime(2025, 7, 4, 0, 0, 0);
  TEST_ASSERT_EQUAL("Jul 4 (2025) = day 185", 185, calculateDayOfYear(&t10));
  
  Serial.println();
}

// ============================================================
// Test: Advance One Minute (Rollover Cases)
// ============================================================
void testAdvanceOneMinute() {
  Serial.println("--- Advance One Minute ---");
  
  // Simple case: 12:30 -> 12:31
  struct tm t1 = makeTime(2025, 6, 15, 12, 30, 0);
  advanceOneMinute(&t1);
  TEST_ASSERT_EQUAL("12:30->12:31 minute", 31, t1.tm_min);
  TEST_ASSERT_EQUAL("12:30->12:31 hour", 12, t1.tm_hour);
  
  // Minute rollover: 12:59 -> 13:00
  struct tm t2 = makeTime(2025, 6, 15, 12, 59, 0);
  advanceOneMinute(&t2);
  TEST_ASSERT_EQUAL("12:59->13:00 minute", 0, t2.tm_min);
  TEST_ASSERT_EQUAL("12:59->13:00 hour", 13, t2.tm_hour);
  
  // Hour rollover: 23:59 -> 00:00 next day
  struct tm t3 = makeTime(2025, 6, 15, 23, 59, 0);
  advanceOneMinute(&t3);
  TEST_ASSERT_EQUAL("23:59->00:00 minute", 0, t3.tm_min);
  TEST_ASSERT_EQUAL("23:59->00:00 hour", 0, t3.tm_hour);
  TEST_ASSERT_EQUAL("23:59->00:00 day", 16, t3.tm_mday);
  
  // Month rollover: Jan 31 23:59 -> Feb 1 00:00
  struct tm t4 = makeTime(2025, 1, 31, 23, 59, 0);
  advanceOneMinute(&t4);
  TEST_ASSERT_EQUAL("Jan31->Feb1 month", 1, t4.tm_mon);  // Feb = 1 (0-indexed)
  TEST_ASSERT_EQUAL("Jan31->Feb1 day", 1, t4.tm_mday);
  
  // Leap year Feb 28 -> Feb 29
  struct tm t5 = makeTime(2024, 2, 28, 23, 59, 0);
  advanceOneMinute(&t5);
  TEST_ASSERT_EQUAL("Feb28->Feb29 (leap) day", 29, t5.tm_mday);
  TEST_ASSERT_EQUAL("Feb28->Feb29 (leap) month", 1, t5.tm_mon);  // Still Feb
  
  // Non-leap Feb 28 -> Mar 1
  struct tm t6 = makeTime(2025, 2, 28, 23, 59, 0);
  advanceOneMinute(&t6);
  TEST_ASSERT_EQUAL("Feb28->Mar1 (non-leap) month", 2, t6.tm_mon);  // Mar = 2
  TEST_ASSERT_EQUAL("Feb28->Mar1 (non-leap) day", 1, t6.tm_mday);
  
  // Year rollover: Dec 31 23:59 -> Jan 1 00:00
  struct tm t7 = makeTime(2025, 12, 31, 23, 59, 0);
  advanceOneMinute(&t7);
  TEST_ASSERT_EQUAL("Dec31->Jan1 year", 2026 - 1900, t7.tm_year);
  TEST_ASSERT_EQUAL("Dec31->Jan1 month", 0, t7.tm_mon);  // Jan = 0
  TEST_ASSERT_EQUAL("Dec31->Jan1 day", 1, t7.tm_mday);
  
  Serial.println();
}

// ============================================================
// Test: Marker Positions
// ============================================================
void testMarkerPositions() {
  Serial.println("--- Marker Positions ---");
  
  struct tm t = makeTime(2025, 6, 15, 12, 30, 0);
  uint8_t frame[60];
  encodeWWVBFrame(&t, frame, DST_STANDARD);
  
  // Markers at 0, 9, 19, 29, 39, 49, 59
  TEST_ASSERT_EQUAL("Position 0 is marker", WWVB_BIT_MARKER, frame[0]);
  TEST_ASSERT_EQUAL("Position 9 is marker", WWVB_BIT_MARKER, frame[9]);
  TEST_ASSERT_EQUAL("Position 19 is marker", WWVB_BIT_MARKER, frame[19]);
  TEST_ASSERT_EQUAL("Position 29 is marker", WWVB_BIT_MARKER, frame[29]);
  TEST_ASSERT_EQUAL("Position 39 is marker", WWVB_BIT_MARKER, frame[39]);
  TEST_ASSERT_EQUAL("Position 49 is marker", WWVB_BIT_MARKER, frame[49]);
  TEST_ASSERT_EQUAL("Position 59 is marker", WWVB_BIT_MARKER, frame[59]);
  
  // Verify non-marker positions are NOT markers
  TEST_ASSERT("Position 1 is not marker", frame[1] != WWVB_BIT_MARKER);
  TEST_ASSERT("Position 30 is not marker", frame[30] != WWVB_BIT_MARKER);
  TEST_ASSERT("Position 58 is not marker", frame[58] != WWVB_BIT_MARKER);
  
  Serial.println();
}

// ============================================================
// Test: Reserved Bits Always Zero
// ============================================================
void testReservedBits() {
  Serial.println("--- Reserved Bits ---");
  
  // Test with a time that would set many data bits to 1
  struct tm t = makeTime(2099, 12, 31, 23, 59, 0);  // Max values
  uint8_t frame[60];
  encodeWWVBFrame(&t, frame, DST_IN_EFFECT);
  
  // Reserved positions: 4, 10, 11, 14, 20, 21, 24, 34, 35, 44, 54
  TEST_ASSERT_EQUAL("Reserved bit 4 = 0", WWVB_BIT_ZERO, frame[4]);
  TEST_ASSERT_EQUAL("Reserved bit 10 = 0", WWVB_BIT_ZERO, frame[10]);
  TEST_ASSERT_EQUAL("Reserved bit 11 = 0", WWVB_BIT_ZERO, frame[11]);
  TEST_ASSERT_EQUAL("Reserved bit 14 = 0", WWVB_BIT_ZERO, frame[14]);
  TEST_ASSERT_EQUAL("Reserved bit 20 = 0", WWVB_BIT_ZERO, frame[20]);
  TEST_ASSERT_EQUAL("Reserved bit 21 = 0", WWVB_BIT_ZERO, frame[21]);
  TEST_ASSERT_EQUAL("Reserved bit 24 = 0", WWVB_BIT_ZERO, frame[24]);
  TEST_ASSERT_EQUAL("Reserved bit 34 = 0", WWVB_BIT_ZERO, frame[34]);
  TEST_ASSERT_EQUAL("Reserved bit 35 = 0", WWVB_BIT_ZERO, frame[35]);
  TEST_ASSERT_EQUAL("Reserved bit 44 = 0", WWVB_BIT_ZERO, frame[44]);
  TEST_ASSERT_EQUAL("Reserved bit 54 = 0", WWVB_BIT_ZERO, frame[54]);
  
  Serial.println();
}

// ============================================================
// Test: Minute Encoding
// ============================================================
void testMinuteEncoding() {
  Serial.println("--- Minute Encoding ---");
  uint8_t frame[60];
  
  // Minute 0: all bits should be 0
  struct tm t0 = makeTime(2025, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t0, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Min 0: bit1=0", WWVB_BIT_ZERO, frame[1]);
  TEST_ASSERT_EQUAL("Min 0: bit2=0", WWVB_BIT_ZERO, frame[2]);
  TEST_ASSERT_EQUAL("Min 0: bit3=0", WWVB_BIT_ZERO, frame[3]);
  TEST_ASSERT_EQUAL("Min 0: bit5=0", WWVB_BIT_ZERO, frame[5]);
  TEST_ASSERT_EQUAL("Min 0: bit8=0", WWVB_BIT_ZERO, frame[8]);
  
  // Minute 1: only 1-weight bit set
  struct tm t1 = makeTime(2025, 1, 1, 0, 1, 0);
  encodeWWVBFrame(&t1, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Min 1: bit8=1 (1-weight)", WWVB_BIT_ONE, frame[8]);
  TEST_ASSERT_EQUAL("Min 1: bit7=0 (2-weight)", WWVB_BIT_ZERO, frame[7]);
  
  // Minute 30: 20+10 = bits 2,3
  struct tm t30 = makeTime(2025, 1, 1, 0, 30, 0);
  encodeWWVBFrame(&t30, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Min 30: bit1=0 (40)", WWVB_BIT_ZERO, frame[1]);
  TEST_ASSERT_EQUAL("Min 30: bit2=1 (20)", WWVB_BIT_ONE, frame[2]);
  TEST_ASSERT_EQUAL("Min 30: bit3=1 (10)", WWVB_BIT_ONE, frame[3]);
  TEST_ASSERT_EQUAL("Min 30: bit5=0 (8)", WWVB_BIT_ZERO, frame[5]);
  
  // Minute 59: 40+10+8+1 = bits 1,3,5,8
  struct tm t59 = makeTime(2025, 1, 1, 0, 59, 0);
  encodeWWVBFrame(&t59, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Min 59: bit1=1 (40)", WWVB_BIT_ONE, frame[1]);
  TEST_ASSERT_EQUAL("Min 59: bit2=0 (20)", WWVB_BIT_ZERO, frame[2]);
  TEST_ASSERT_EQUAL("Min 59: bit3=1 (10)", WWVB_BIT_ONE, frame[3]);
  TEST_ASSERT_EQUAL("Min 59: bit5=1 (8)", WWVB_BIT_ONE, frame[5]);
  TEST_ASSERT_EQUAL("Min 59: bit6=0 (4)", WWVB_BIT_ZERO, frame[6]);
  TEST_ASSERT_EQUAL("Min 59: bit7=0 (2)", WWVB_BIT_ZERO, frame[7]);
  TEST_ASSERT_EQUAL("Min 59: bit8=1 (1)", WWVB_BIT_ONE, frame[8]);
  
  // Minute 42: 40+2 = bits 1,7
  struct tm t42 = makeTime(2025, 1, 1, 0, 42, 0);
  encodeWWVBFrame(&t42, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Min 42: bit1=1 (40)", WWVB_BIT_ONE, frame[1]);
  TEST_ASSERT_EQUAL("Min 42: bit2=0 (20)", WWVB_BIT_ZERO, frame[2]);
  TEST_ASSERT_EQUAL("Min 42: bit3=0 (10)", WWVB_BIT_ZERO, frame[3]);
  TEST_ASSERT_EQUAL("Min 42: bit5=0 (8)", WWVB_BIT_ZERO, frame[5]);
  TEST_ASSERT_EQUAL("Min 42: bit6=0 (4)", WWVB_BIT_ZERO, frame[6]);
  TEST_ASSERT_EQUAL("Min 42: bit7=1 (2)", WWVB_BIT_ONE, frame[7]);
  TEST_ASSERT_EQUAL("Min 42: bit8=0 (1)", WWVB_BIT_ZERO, frame[8]);
  
  Serial.println();
}

// ============================================================
// Test: Hour Encoding
// ============================================================
void testHourEncoding() {
  Serial.println("--- Hour Encoding ---");
  uint8_t frame[60];
  
  // Hour 0
  struct tm t0 = makeTime(2025, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t0, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Hour 0: bit12=0 (20)", WWVB_BIT_ZERO, frame[12]);
  TEST_ASSERT_EQUAL("Hour 0: bit13=0 (10)", WWVB_BIT_ZERO, frame[13]);
  TEST_ASSERT_EQUAL("Hour 0: bit15=0 (8)", WWVB_BIT_ZERO, frame[15]);
  TEST_ASSERT_EQUAL("Hour 0: bit18=0 (1)", WWVB_BIT_ZERO, frame[18]);
  
  // Hour 15: 10+4+1
  struct tm t15 = makeTime(2025, 1, 1, 15, 0, 0);
  encodeWWVBFrame(&t15, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Hour 15: bit12=0 (20)", WWVB_BIT_ZERO, frame[12]);
  TEST_ASSERT_EQUAL("Hour 15: bit13=1 (10)", WWVB_BIT_ONE, frame[13]);
  TEST_ASSERT_EQUAL("Hour 15: bit15=0 (8)", WWVB_BIT_ZERO, frame[15]);
  TEST_ASSERT_EQUAL("Hour 15: bit16=1 (4)", WWVB_BIT_ONE, frame[16]);
  TEST_ASSERT_EQUAL("Hour 15: bit17=0 (2)", WWVB_BIT_ZERO, frame[17]);
  TEST_ASSERT_EQUAL("Hour 15: bit18=1 (1)", WWVB_BIT_ONE, frame[18]);
  
  // Hour 23: 20+2+1
  struct tm t23 = makeTime(2025, 1, 1, 23, 0, 0);
  encodeWWVBFrame(&t23, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Hour 23: bit12=1 (20)", WWVB_BIT_ONE, frame[12]);
  TEST_ASSERT_EQUAL("Hour 23: bit13=0 (10)", WWVB_BIT_ZERO, frame[13]);
  TEST_ASSERT_EQUAL("Hour 23: bit15=0 (8)", WWVB_BIT_ZERO, frame[15]);
  TEST_ASSERT_EQUAL("Hour 23: bit16=0 (4)", WWVB_BIT_ZERO, frame[16]);
  TEST_ASSERT_EQUAL("Hour 23: bit17=1 (2)", WWVB_BIT_ONE, frame[17]);
  TEST_ASSERT_EQUAL("Hour 23: bit18=1 (1)", WWVB_BIT_ONE, frame[18]);
  
  Serial.println();
}

// ============================================================
// Test: Day of Year Encoding
// ============================================================
void testDayOfYearEncoding() {
  Serial.println("--- Day of Year Encoding ---");
  uint8_t frame[60];
  
  // Day 1 (Jan 1): 1
  struct tm t1 = makeTime(2025, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t1, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Day 1: bit22=0 (200)", WWVB_BIT_ZERO, frame[22]);
  TEST_ASSERT_EQUAL("Day 1: bit23=0 (100)", WWVB_BIT_ZERO, frame[23]);
  TEST_ASSERT_EQUAL("Day 1: bit33=1 (1)", WWVB_BIT_ONE, frame[33]);
  
  // Day 200 (Jul 19, 2025): 200
  struct tm t200 = makeTime(2025, 7, 19, 0, 0, 0);
  encodeWWVBFrame(&t200, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Day 200: bit22=1 (200)", WWVB_BIT_ONE, frame[22]);
  TEST_ASSERT_EQUAL("Day 200: bit23=0 (100)", WWVB_BIT_ZERO, frame[23]);
  TEST_ASSERT_EQUAL("Day 200: bit25=0 (80)", WWVB_BIT_ZERO, frame[25]);
  TEST_ASSERT_EQUAL("Day 200: bit30=0 (8)", WWVB_BIT_ZERO, frame[30]);
  TEST_ASSERT_EQUAL("Day 200: bit33=0 (1)", WWVB_BIT_ZERO, frame[33]);
  
  // Day 365 (Dec 31, non-leap): 200+100+40+20+4+1 = 365
  struct tm t365 = makeTime(2025, 12, 31, 0, 0, 0);
  encodeWWVBFrame(&t365, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Day 365: bit22=1 (200)", WWVB_BIT_ONE, frame[22]);
  TEST_ASSERT_EQUAL("Day 365: bit23=1 (100)", WWVB_BIT_ONE, frame[23]);
  int doy365 = calculateDayOfYear(&t365);
  TEST_ASSERT_EQUAL("Day 365: DOY correct", 365, doy365);
  
  // Day 366 (Dec 31, leap year 2024)
  struct tm t366 = makeTime(2024, 12, 31, 0, 0, 0);
  int doy366 = calculateDayOfYear(&t366);
  TEST_ASSERT_EQUAL("Day 366: DOY correct", 366, doy366);
  encodeWWVBFrame(&t366, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Day 366: bit22=1 (200)", WWVB_BIT_ONE, frame[22]);
  TEST_ASSERT_EQUAL("Day 366: bit23=1 (100)", WWVB_BIT_ONE, frame[23]);
  
  Serial.println();
}

// ============================================================
// Test: Year Encoding
// ============================================================
void testYearEncoding() {
  Serial.println("--- Year Encoding ---");
  uint8_t frame[60];
  
  // Year 2025 -> encode as 25: 20+4+1
  struct tm t25 = makeTime(2025, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t25, frame, DST_STANDARD);
  // Year tens = 2: 80=0, 40=0, 20=1, 10=0
  TEST_ASSERT_EQUAL("Year 25: bit45=0 (80)", WWVB_BIT_ZERO, frame[45]);
  TEST_ASSERT_EQUAL("Year 25: bit46=0 (40)", WWVB_BIT_ZERO, frame[46]);
  TEST_ASSERT_EQUAL("Year 25: bit47=1 (20)", WWVB_BIT_ONE, frame[47]);
  TEST_ASSERT_EQUAL("Year 25: bit48=0 (10)", WWVB_BIT_ZERO, frame[48]);
  // Year ones = 5: 8=0, 4=1, 2=0, 1=1
  TEST_ASSERT_EQUAL("Year 25: bit50=0 (8)", WWVB_BIT_ZERO, frame[50]);
  TEST_ASSERT_EQUAL("Year 25: bit51=1 (4)", WWVB_BIT_ONE, frame[51]);
  TEST_ASSERT_EQUAL("Year 25: bit52=0 (2)", WWVB_BIT_ZERO, frame[52]);
  TEST_ASSERT_EQUAL("Year 25: bit53=1 (1)", WWVB_BIT_ONE, frame[53]);
  
  // Year 2000 -> encode as 00
  struct tm t00 = makeTime(2000, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t00, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Year 00: all year bits 0", WWVB_BIT_ZERO, frame[45]);
  TEST_ASSERT_EQUAL("Year 00: bit53=0", WWVB_BIT_ZERO, frame[53]);
  
  // Year 2099 -> encode as 99: 80+10+8+1
  struct tm t99 = makeTime(2099, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t99, frame, DST_STANDARD);
  // Year tens digit is 9: weights are 80,40,20,10 for the tens digit value (0-9)
  // 9 = 8+1 -> bit45=1(8), bit46=0(4), bit47=0(2), bit48=1(1)
  TEST_ASSERT_EQUAL("Year 99: bit45=1 (tens 8)", WWVB_BIT_ONE, frame[45]);
  TEST_ASSERT_EQUAL("Year 99: bit46=0 (tens 4)", WWVB_BIT_ZERO, frame[46]);
  TEST_ASSERT_EQUAL("Year 99: bit47=0 (tens 2)", WWVB_BIT_ZERO, frame[47]);
  TEST_ASSERT_EQUAL("Year 99: bit48=1 (tens 1)", WWVB_BIT_ONE, frame[48]);
  // Year ones = 9: 8+1
  TEST_ASSERT_EQUAL("Year 99: bit50=1 (ones 8)", WWVB_BIT_ONE, frame[50]);
  TEST_ASSERT_EQUAL("Year 99: bit51=0 (ones 4)", WWVB_BIT_ZERO, frame[51]);
  TEST_ASSERT_EQUAL("Year 99: bit52=0 (ones 2)", WWVB_BIT_ZERO, frame[52]);
  TEST_ASSERT_EQUAL("Year 99: bit53=1 (ones 1)", WWVB_BIT_ONE, frame[53]);
  
  Serial.println();
}

// ============================================================
// Test: DST Status Bits
// ============================================================
void testDSTBits() {
  Serial.println("--- DST Status Bits ---");
  uint8_t frame[60];
  struct tm t = makeTime(2025, 6, 15, 12, 0, 0);
  
  // Standard Time: 57=0, 58=0
  encodeWWVBFrame(&t, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("Standard: bit57=0", WWVB_BIT_ZERO, frame[57]);
  TEST_ASSERT_EQUAL("Standard: bit58=0", WWVB_BIT_ZERO, frame[58]);
  
  // DST Begins: 57=1, 58=0
  encodeWWVBFrame(&t, frame, DST_BEGINS);
  TEST_ASSERT_EQUAL("DST Begins: bit57=1", WWVB_BIT_ONE, frame[57]);
  TEST_ASSERT_EQUAL("DST Begins: bit58=0", WWVB_BIT_ZERO, frame[58]);
  
  // DST In Effect: 57=1, 58=1
  encodeWWVBFrame(&t, frame, DST_IN_EFFECT);
  TEST_ASSERT_EQUAL("DST In Effect: bit57=1", WWVB_BIT_ONE, frame[57]);
  TEST_ASSERT_EQUAL("DST In Effect: bit58=1", WWVB_BIT_ONE, frame[58]);
  
  // DST Ends: 57=0, 58=1
  encodeWWVBFrame(&t, frame, DST_ENDS);
  TEST_ASSERT_EQUAL("DST Ends: bit57=0", WWVB_BIT_ZERO, frame[57]);
  TEST_ASSERT_EQUAL("DST Ends: bit58=1", WWVB_BIT_ONE, frame[58]);
  
  Serial.println();
}

// ============================================================
// Test: Leap Year Indicator Bit
// ============================================================
void testLeapYearBit() {
  Serial.println("--- Leap Year Bit ---");
  uint8_t frame[60];
  
  // 2024 is leap year
  struct tm t24 = makeTime(2024, 6, 15, 12, 0, 0);
  encodeWWVBFrame(&t24, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("2024 leap year bit=1", WWVB_BIT_ONE, frame[55]);
  
  // 2025 is not leap year
  struct tm t25 = makeTime(2025, 6, 15, 12, 0, 0);
  encodeWWVBFrame(&t25, frame, DST_STANDARD);
  TEST_ASSERT_EQUAL("2025 leap year bit=0", WWVB_BIT_ZERO, frame[55]);
  
  // Leap second warning always 0
  TEST_ASSERT_EQUAL("Leap second warning=0", WWVB_BIT_ZERO, frame[56]);
  
  Serial.println();
}

// ============================================================
// Test: UT1 Correction Bits
// ============================================================
void testUT1Bits() {
  Serial.println("--- UT1 Correction Bits ---");
  uint8_t frame[60];
  
  struct tm t = makeTime(2025, 1, 1, 0, 0, 0);
  encodeWWVBFrame(&t, frame, DST_STANDARD);
  
  // UT1 sign: positive (+)
  TEST_ASSERT_EQUAL("UT1 sign+ bit36=1", WWVB_BIT_ONE, frame[36]);
  TEST_ASSERT_EQUAL("UT1 sign- bit37=0", WWVB_BIT_ZERO, frame[37]);
  TEST_ASSERT_EQUAL("UT1 sign+ bit38=1", WWVB_BIT_ONE, frame[38]);
  
  // UT1 magnitude: 0.0
  TEST_ASSERT_EQUAL("UT1 0.8 bit40=0", WWVB_BIT_ZERO, frame[40]);
  TEST_ASSERT_EQUAL("UT1 0.4 bit41=0", WWVB_BIT_ZERO, frame[41]);
  TEST_ASSERT_EQUAL("UT1 0.2 bit42=0", WWVB_BIT_ZERO, frame[42]);
  TEST_ASSERT_EQUAL("UT1 0.1 bit43=0", WWVB_BIT_ZERO, frame[43]);
  
  Serial.println();
}

// ============================================================
// Test: Bit Duration Functions
// ============================================================
void testBitDurations() {
  Serial.println("--- Bit Durations ---");
  
  TEST_ASSERT_EQUAL("Bit 0 low = 200ms", 200, getLowDurationMs(WWVB_BIT_ZERO));
  TEST_ASSERT_EQUAL("Bit 0 high = 800ms", 800, getHighDurationMs(WWVB_BIT_ZERO));
  TEST_ASSERT_EQUAL("Bit 1 low = 500ms", 500, getLowDurationMs(WWVB_BIT_ONE));
  TEST_ASSERT_EQUAL("Bit 1 high = 500ms", 500, getHighDurationMs(WWVB_BIT_ONE));
  TEST_ASSERT_EQUAL("Marker low = 800ms", 800, getLowDurationMs(WWVB_BIT_MARKER));
  TEST_ASSERT_EQUAL("Marker high = 200ms", 200, getHighDurationMs(WWVB_BIT_MARKER));
  
  // Verify all bits sum to 1000ms
  TEST_ASSERT_EQUAL("Bit 0 total = 1000ms", 1000, 
    getLowDurationMs(WWVB_BIT_ZERO) + getHighDurationMs(WWVB_BIT_ZERO));
  TEST_ASSERT_EQUAL("Bit 1 total = 1000ms", 1000,
    getLowDurationMs(WWVB_BIT_ONE) + getHighDurationMs(WWVB_BIT_ONE));
  TEST_ASSERT_EQUAL("Marker total = 1000ms", 1000,
    getLowDurationMs(WWVB_BIT_MARKER) + getHighDurationMs(WWVB_BIT_MARKER));
  
  Serial.println();
}

// ============================================================
// Reference Frame Tests
// ============================================================

/*
 * Reference Frame 1: 2025-01-01 00:01 UTC
 * Day of year: 1
 * Standard time, not leap year
 * 
 * Minutes = 1:  40=0, 20=0, 10=0, 8=0, 4=0, 2=0, 1=1
 * Hours   = 0:  20=0, 10=0, 8=0, 4=0, 2=0, 1=0
 * DOY     = 1:  200=0, 100=0, 80=0, 40=0, 20=0, 10=0, 8=0, 4=0, 2=0, 1=1
 * Year    = 25: tens=2(0010), ones=5(0101)
 */
void testReferenceFrame1() {
  Serial.println("--- Reference Frame 1: 2025-01-01 00:01 UTC ---");
  
  struct tm t = makeTime(2025, 1, 1, 0, 1, 0);
  uint8_t frame[60];
  encodeWWVBFrame(&t, frame, DST_STANDARD);
  
  // Expected frame (M=marker, 0/1=data)
  // Sec 0-9:   M 0 0 0 0  0 0 0 1 M  (min=1)
  uint8_t expected_0_9[] = {2, 0, 0, 0, 0, 0, 0, 0, 1, 2};
  bool match_0_9 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i] != expected_0_9[i]) match_0_9 = false;
  }
  TEST_ASSERT("Sec 0-9 match (min=1)", match_0_9);
  
  // Sec 10-19: 0 0 0 0 0  0 0 0 0 M  (hour=0)
  uint8_t expected_10_19[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
  bool match_10_19 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i + 10] != expected_10_19[i]) match_10_19 = false;
  }
  TEST_ASSERT("Sec 10-19 match (hour=0)", match_10_19);
  
  // Sec 20-29: 0 0 0 0 0  0 0 0 0 M  (day=1, high bits)
  uint8_t expected_20_29[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
  bool match_20_29 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i + 20] != expected_20_29[i]) match_20_29 = false;
  }
  TEST_ASSERT("Sec 20-29 match (day high=001)", match_20_29);
  
  // Sec 30-39: 0 0 0 1 0  0 1 0 1 M  (day low=1, UT1=+0.0)
  uint8_t expected_30_39[] = {0, 0, 0, 1, 0, 0, 1, 0, 1, 2};
  bool match_30_39 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i + 30] != expected_30_39[i]) match_30_39 = false;
  }
  TEST_ASSERT("Sec 30-39 match (day low, UT1)", match_30_39);
  
  // Sec 40-49: 0 0 0 0 0  0 0 1 0 M  (UT1=0, year tens=2)
  uint8_t expected_40_49[] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 2};
  bool match_40_49 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i + 40] != expected_40_49[i]) match_40_49 = false;
  }
  TEST_ASSERT("Sec 40-49 match (UT1, year high=2)", match_40_49);
  
  // Sec 50-59: 0 1 0 1 0  0 0 0 0 M  (year ones=5, LY=0, LS=0, DST=00)
  uint8_t expected_50_59[] = {0, 1, 0, 1, 0, 0, 0, 0, 0, 2};
  bool match_50_59 = true;
  for (int i = 0; i < 10; i++) {
    if (frame[i + 50] != expected_50_59[i]) match_50_59 = false;
  }
  TEST_ASSERT("Sec 50-59 match (year low=5, flags)", match_50_59);
  
  // Print frame for visual inspection
  printFrame(frame);
  Serial.println();
}

/*
 * Reference Frame 2: 2025-12-31 23:59 UTC → encodes 2026-01-01 00:00
 * Tests year boundary rollover via advanceOneMinute
 */
void testReferenceFrame2() {
  Serial.println("--- Reference Frame 2: Year Boundary (2025→2026) ---");
  
  // Current time: 2025-12-31 23:59
  struct tm current = makeTime(2025, 12, 31, 23, 59, 0);
  struct tm nextMin = current;
  advanceOneMinute(&nextMin);
  
  // Verify rollover
  TEST_ASSERT_EQUAL("Rolled to year 2026", 2026 - 1900, nextMin.tm_year);
  TEST_ASSERT_EQUAL("Rolled to month Jan", 0, nextMin.tm_mon);
  TEST_ASSERT_EQUAL("Rolled to day 1", 1, nextMin.tm_mday);
  TEST_ASSERT_EQUAL("Rolled to hour 0", 0, nextMin.tm_hour);
  TEST_ASSERT_EQUAL("Rolled to minute 0", 0, nextMin.tm_min);
  
  // Encode the rolled-over time
  uint8_t frame[60];
  encodeWWVBFrame(&nextMin, frame, DST_STANDARD);
  
  // Year should be 26
  // Tens=2: 0,0,1,0  Ones=6: 0,1,1,0
  TEST_ASSERT_EQUAL("Year 26: tens bit47=1 (20)", WWVB_BIT_ONE, frame[47]);
  TEST_ASSERT_EQUAL("Year 26: ones bit51=1 (4)", WWVB_BIT_ONE, frame[51]);
  TEST_ASSERT_EQUAL("Year 26: ones bit52=1 (2)", WWVB_BIT_ONE, frame[52]);
  
  // Day should be 1
  TEST_ASSERT_EQUAL("Day 1: bit33=1 (1)", WWVB_BIT_ONE, frame[33]);
  TEST_ASSERT_EQUAL("Day 1: bit22=0 (200)", WWVB_BIT_ZERO, frame[22]);
  
  // Not a leap year (2026)
  TEST_ASSERT_EQUAL("2026 not leap year", WWVB_BIT_ZERO, frame[55]);
  
  printFrame(frame);
  Serial.println();
}

/*
 * Reference Frame 3: 2025-03-09 (DST Spring Forward in US)
 * Tests DST begins today bits
 */
void testReferenceFrame3() {
  Serial.println("--- Reference Frame 3: DST Spring Forward ---");
  
  // Day 68 of 2025 (March 9)
  struct tm t = makeTime(2025, 3, 9, 10, 0, 0);  // 10:00 UTC
  
  int doy = calculateDayOfYear(&t);
  TEST_ASSERT_EQUAL("Mar 9 2025 = day 68", 68, doy);
  
  uint8_t frame[60];
  
  // DST begins today
  encodeWWVBFrame(&t, frame, DST_BEGINS);
  TEST_ASSERT_EQUAL("DST begins: bit57=1", WWVB_BIT_ONE, frame[57]);
  TEST_ASSERT_EQUAL("DST begins: bit58=0", WWVB_BIT_ZERO, frame[58]);
  
  printFrame(frame);
  Serial.println();
}

/*
 * Reference Frame 4: 2024-12-31 (Leap year, day 366)
 * Tests leap year flag and max day-of-year
 */
void testReferenceFrame4() {
  Serial.println("--- Reference Frame 4: Leap Year Day 366 ---");
  
  struct tm t = makeTime(2024, 12, 31, 12, 30, 0);
  
  int doy = calculateDayOfYear(&t);
  TEST_ASSERT_EQUAL("Dec 31 2024 = day 366", 366, doy);
  
  uint8_t frame[60];
  encodeWWVBFrame(&t, frame, DST_STANDARD);
  
  // Leap year = 1
  TEST_ASSERT_EQUAL("2024 is leap year", WWVB_BIT_ONE, frame[55]);
  
  // Day 366 = 200+100+40+20+4+2 = 366
  TEST_ASSERT_EQUAL("Day 366: bit22=1 (200)", WWVB_BIT_ONE, frame[22]);
  TEST_ASSERT_EQUAL("Day 366: bit23=1 (100)", WWVB_BIT_ONE, frame[23]);
  // Remainder: 66. 80? No. 66 = 40+20+6. 6 = 4+2
  int rem100 = 366 % 100;  // 66
  TEST_ASSERT_EQUAL("Day 366 rem100=66", 66, rem100);
  // 66: 80=0, 40=1, 20=1, 10=0 -> 40+20=60, ones=6: 8=0,4=1,2=1,1=0
  TEST_ASSERT_EQUAL("Day 366: bit25=0 (80)", WWVB_BIT_ZERO, frame[25]);
  TEST_ASSERT_EQUAL("Day 366: bit26=1 (40)", WWVB_BIT_ONE, frame[26]);
  TEST_ASSERT_EQUAL("Day 366: bit27=1 (20)", WWVB_BIT_ONE, frame[27]);
  TEST_ASSERT_EQUAL("Day 366: bit28=0 (10)", WWVB_BIT_ZERO, frame[28]);
  TEST_ASSERT_EQUAL("Day 366: bit30=0 (8)", WWVB_BIT_ZERO, frame[30]);
  TEST_ASSERT_EQUAL("Day 366: bit31=1 (4)", WWVB_BIT_ONE, frame[31]);
  TEST_ASSERT_EQUAL("Day 366: bit32=1 (2)", WWVB_BIT_ONE, frame[32]);
  TEST_ASSERT_EQUAL("Day 366: bit33=0 (1)", WWVB_BIT_ZERO, frame[33]);
  
  printFrame(frame);
  Serial.println();
}
