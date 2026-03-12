/*
 * test_dst_calculation.ino - DST Automatic Calculation Tests
 * 
 * Comprehensive tests for the automatic DST status calculator.
 * Validates day-of-week, nth weekday, transition dates, and
 * full DST status calculation for US timezones.
 * 
 * NO HARDWARE REQUIRED - pure logic tests only.
 * FULLY SELF-CONTAINED - no external files needed.
 * Upload to ESP32, open Serial Monitor at 115200 baud.
 */

// ============================================================
// DST Manager (inlined from wwvb_transmitter/dst_manager.h/.cpp)
// Source of truth: wwvb_transmitter/dst_manager.h/.cpp
// Last synced: 2026-03-07
// ============================================================

#include <Arduino.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// DST status values (matches wwvb_encoder.h constants)
#define DST_STANDARD    0x00  // Standard Time in effect
#define DST_BEGINS      0x02  // DST begins today
#define DST_IN_EFFECT   0x03  // DST in effect
#define DST_ENDS        0x01  // DST ends today

// US timezone UTC offsets (standard time)
enum USTimezone {
  TZ_US_EASTERN  = -5,   // EST: UTC-5, EDT: UTC-4
  TZ_US_CENTRAL  = -6,   // CST: UTC-6, CDT: UTC-5
  TZ_US_MOUNTAIN = -7,   // MST: UTC-7, MDT: UTC-6
  TZ_US_PACIFIC  = -8    // PST: UTC-8, PDT: UTC-7
};

// Days in each month (non-leap year)
static const int MONTH_DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 * Day of week using Tomohiko Sakamoto's algorithm.
 * Returns 0=Sunday, 1=Monday, ..., 6=Saturday.
 * Valid for any Gregorian date.
 */
int dayOfWeek(int year, int month, int day) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) year--;
  return (year + year/4 - year/100 + year/400 + t[month - 1] + day) % 7;
}

/*
 * Find the Nth occurrence of a weekday in a given month/year.
 * weekday: 0=Sunday
 * n: 1=first, 2=second, etc.
 */
int nthWeekdayOfMonth(int year, int month, int weekday, int n) {
  int dow1 = dayOfWeek(year, month, 1);
  int firstOccurrence = 1 + ((weekday - dow1 + 7) % 7);
  int day = firstOccurrence + (n - 1) * 7;
  return day;
}

/*
 * DST start: 2nd Sunday in March
 */
int getDSTStartDay(int year) {
  return nthWeekdayOfMonth(year, 3, 0, 2);
}

/*
 * DST end: 1st Sunday in November
 */
int getDSTEndDay(int year) {
  return nthWeekdayOfMonth(year, 11, 0, 1);
}

/*
 * Check if a local date/time falls within DST period.
 * Handles the transition hours on boundary days.
 * month: 1-12, day: 1-31, hour: 0-23 (local standard time hour)
 */
int isInDST(int year, int month, int day, int hour) {
  int dstStartDay = getDSTStartDay(year);
  int dstEndDay = getDSTEndDay(year);
  
  if (month < 3 || month > 11) return 0;
  if (month > 3 && month < 11) return 1;
  
  if (month == 3) {
    if (day < dstStartDay) return 0;
    if (day > dstStartDay) return 1;
    return (hour >= 2) ? 1 : 0;
  }
  
  if (month == 11) {
    if (day < dstEndDay) return 1;
    if (day > dstEndDay) return 0;
    return (hour < 1) ? 1 : 0;
  }
  
  return 0;
}

/*
 * Calculate the WWVB DST status bits from UTC time and timezone.
 */
uint8_t calculateDSTStatus(const struct tm* utcTime, int timezoneOffset) {
  int localHour = utcTime->tm_hour + timezoneOffset;
  int localDay = utcTime->tm_mday;
  int localMonth = utcTime->tm_mon + 1;
  int localYear = utcTime->tm_year + 1900;
  
  if (localHour < 0) {
    localHour += 24;
    localDay--;
    if (localDay < 1) {
      localMonth--;
      if (localMonth < 1) {
        localMonth = 12;
        localYear--;
      }
      int prevMonthIdx = localMonth - 1;
      localDay = MONTH_DAYS[prevMonthIdx];
      if (prevMonthIdx == 1 && ((localYear % 4 == 0 && localYear % 100 != 0) || localYear % 400 == 0)) {
        localDay = 29;
      }
    }
  }
  
  int dstStartDay = getDSTStartDay(localYear);
  int dstEndDay = getDSTEndDay(localYear);
  
  if (localMonth == 3 && localDay == dstStartDay) {
    return DST_BEGINS;
  }
  
  if (localMonth == 11 && localDay == dstEndDay) {
    return DST_ENDS;
  }
  
  if (isInDST(localYear, localMonth, localDay, localHour)) {
    return DST_IN_EFFECT;
  }
  
  return DST_STANDARD;
}

// ============================================================
// End of inlined DST manager
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

// Helper: create a UTC struct tm
struct tm makeUTC(int year, int month, int day, int hour, int minute) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = 0;
  return t;
}

// ============================================================
// Setup - Run All Tests
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  DST Calculation Test Suite");
  Serial.println("========================================");
  Serial.println();
  
  testDayOfWeek();
  testNthWeekday();
  testDSTTransitionDates();
  testIsInDST();
  testCalculateDSTStatus_Pacific();
  testCalculateDSTStatus_Eastern();
  testUTCToLocalRollback();
  testMultiYearTransitions();
  testEdgeCases();
  
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
  delay(10000);
}

// ============================================================
// Test: Day of Week (Sakamoto's Algorithm)
// ============================================================
void testDayOfWeek() {
  Serial.println("--- Day of Week ---");
  
  // Known dates (verified against calendar)
  TEST_ASSERT_EQUAL("2025-01-01 = Wednesday", 3, dayOfWeek(2025, 1, 1));
  TEST_ASSERT_EQUAL("2025-03-09 = Sunday", 0, dayOfWeek(2025, 3, 9));
  TEST_ASSERT_EQUAL("2025-11-02 = Sunday", 0, dayOfWeek(2025, 11, 2));
  TEST_ASSERT_EQUAL("2024-01-01 = Monday", 1, dayOfWeek(2024, 1, 1));
  TEST_ASSERT_EQUAL("2024-02-29 = Thursday", 4, dayOfWeek(2024, 2, 29));
  TEST_ASSERT_EQUAL("2000-01-01 = Saturday", 6, dayOfWeek(2000, 1, 1));
  TEST_ASSERT_EQUAL("2026-01-01 = Thursday", 4, dayOfWeek(2026, 1, 1));
  TEST_ASSERT_EQUAL("2026-03-08 = Sunday", 0, dayOfWeek(2026, 3, 8));
  TEST_ASSERT_EQUAL("2026-11-01 = Sunday", 0, dayOfWeek(2026, 11, 1));
  
  Serial.println();
}

// ============================================================
// Test: Nth Weekday of Month
// ============================================================
void testNthWeekday() {
  Serial.println("--- Nth Weekday of Month ---");
  
  // 2nd Sunday in March 2025 = March 9
  TEST_ASSERT_EQUAL("2nd Sun Mar 2025 = 9", 9, nthWeekdayOfMonth(2025, 3, 0, 2));
  
  // 1st Sunday in November 2025 = November 2
  TEST_ASSERT_EQUAL("1st Sun Nov 2025 = 2", 2, nthWeekdayOfMonth(2025, 11, 0, 1));
  
  // 2nd Sunday in March 2026 = March 8
  TEST_ASSERT_EQUAL("2nd Sun Mar 2026 = 8", 8, nthWeekdayOfMonth(2026, 3, 0, 2));
  
  // 1st Sunday in November 2026 = November 1
  TEST_ASSERT_EQUAL("1st Sun Nov 2026 = 1", 1, nthWeekdayOfMonth(2026, 11, 0, 1));
  
  // 2nd Sunday in March 2024 = March 10
  TEST_ASSERT_EQUAL("2nd Sun Mar 2024 = 10", 10, nthWeekdayOfMonth(2024, 3, 0, 2));
  
  // 1st Sunday in November 2024 = November 3
  TEST_ASSERT_EQUAL("1st Sun Nov 2024 = 3", 3, nthWeekdayOfMonth(2024, 11, 0, 1));
  
  // Edge: 1st Monday in January 2025 = January 6
  TEST_ASSERT_EQUAL("1st Mon Jan 2025 = 6", 6, nthWeekdayOfMonth(2025, 1, 1, 1));
  
  Serial.println();
}

// ============================================================
// Test: DST Transition Dates
// ============================================================
void testDSTTransitionDates() {
  Serial.println("--- DST Transition Dates ---");
  
  // Known US DST transitions (verified against timeanddate.com)
  // 2024
  TEST_ASSERT_EQUAL("2024 DST start = Mar 10", 10, getDSTStartDay(2024));
  TEST_ASSERT_EQUAL("2024 DST end = Nov 3", 3, getDSTEndDay(2024));
  
  // 2025
  TEST_ASSERT_EQUAL("2025 DST start = Mar 9", 9, getDSTStartDay(2025));
  TEST_ASSERT_EQUAL("2025 DST end = Nov 2", 2, getDSTEndDay(2025));
  
  // 2026
  TEST_ASSERT_EQUAL("2026 DST start = Mar 8", 8, getDSTStartDay(2026));
  TEST_ASSERT_EQUAL("2026 DST end = Nov 1", 1, getDSTEndDay(2026));
  
  // 2027
  TEST_ASSERT_EQUAL("2027 DST start = Mar 14", 14, getDSTStartDay(2027));
  TEST_ASSERT_EQUAL("2027 DST end = Nov 7", 7, getDSTEndDay(2027));
  
  // 2028
  TEST_ASSERT_EQUAL("2028 DST start = Mar 12", 12, getDSTStartDay(2028));
  TEST_ASSERT_EQUAL("2028 DST end = Nov 5", 5, getDSTEndDay(2028));
  
  // 2029
  TEST_ASSERT_EQUAL("2029 DST start = Mar 11", 11, getDSTStartDay(2029));
  TEST_ASSERT_EQUAL("2029 DST end = Nov 4", 4, getDSTEndDay(2029));
  
  // 2030
  TEST_ASSERT_EQUAL("2030 DST start = Mar 10", 10, getDSTStartDay(2030));
  TEST_ASSERT_EQUAL("2030 DST end = Nov 3", 3, getDSTEndDay(2030));
  
  Serial.println();
}

// ============================================================
// Test: isInDST (Local Time)
// ============================================================
void testIsInDST() {
  Serial.println("--- isInDST (Local Time) ---");
  
  // January: Standard time
  TEST_ASSERT_EQUAL("Jan 15 = standard", 0, isInDST(2025, 1, 15, 12));
  
  // February: Standard time
  TEST_ASSERT_EQUAL("Feb 14 = standard", 0, isInDST(2025, 2, 14, 12));
  
  // March before transition (Mar 8): Standard
  TEST_ASSERT_EQUAL("Mar 8 = standard", 0, isInDST(2025, 3, 8, 12));
  
  // March transition day (Mar 9) before 2 AM: Standard
  TEST_ASSERT_EQUAL("Mar 9 1:00AM = standard", 0, isInDST(2025, 3, 9, 1));
  
  // March transition day (Mar 9) at 2 AM: DST
  TEST_ASSERT_EQUAL("Mar 9 2:00AM = DST", 1, isInDST(2025, 3, 9, 2));
  
  // March transition day (Mar 9) after 2 AM: DST
  TEST_ASSERT_EQUAL("Mar 9 3:00AM = DST", 1, isInDST(2025, 3, 9, 3));
  
  // March after transition (Mar 10): DST
  TEST_ASSERT_EQUAL("Mar 10 = DST", 1, isInDST(2025, 3, 10, 12));
  
  // July: DST
  TEST_ASSERT_EQUAL("Jul 4 = DST", 1, isInDST(2025, 7, 4, 12));
  
  // October: DST
  TEST_ASSERT_EQUAL("Oct 31 = DST", 1, isInDST(2025, 10, 31, 12));
  
  // November before transition (Nov 1): DST
  TEST_ASSERT_EQUAL("Nov 1 = DST", 1, isInDST(2025, 11, 1, 12));
  
  // November transition day (Nov 2) before 1 AM standard: DST
  TEST_ASSERT_EQUAL("Nov 2 0:30AM = DST", 1, isInDST(2025, 11, 2, 0));
  
  // November transition day (Nov 2) at/after 1 AM standard: Standard
  TEST_ASSERT_EQUAL("Nov 2 1:00AM = standard", 0, isInDST(2025, 11, 2, 1));
  
  // November after transition (Nov 3): Standard
  TEST_ASSERT_EQUAL("Nov 3 = standard", 0, isInDST(2025, 11, 3, 12));
  
  // December: Standard
  TEST_ASSERT_EQUAL("Dec 25 = standard", 0, isInDST(2025, 12, 25, 12));
  
  Serial.println();
}

// ============================================================
// Test: Full DST Status Calculation (Pacific Timezone)
// ============================================================
void testCalculateDSTStatus_Pacific() {
  Serial.println("--- DST Status: Pacific (UTC-8) ---");
  
  const int TZ = TZ_US_PACIFIC;  // -8
  
  // ---- Winter: Standard Time ----
  // Jan 15, 2025 20:00 UTC = Jan 15 12:00 PST
  struct tm t1 = makeUTC(2025, 1, 15, 20, 0);
  TEST_ASSERT_EQUAL("Jan 15 20:00 UTC = STANDARD", DST_STANDARD, calculateDSTStatus(&t1, TZ));
  
  // Feb 14, 2025 08:00 UTC = Feb 14 00:00 PST
  struct tm t2 = makeUTC(2025, 2, 14, 8, 0);
  TEST_ASSERT_EQUAL("Feb 14 08:00 UTC = STANDARD", DST_STANDARD, calculateDSTStatus(&t2, TZ));
  
  // ---- Spring Forward Day: March 9, 2025 ----
  // Mar 9, 2025 08:00 UTC = Mar 9 00:00 PST (transition day, before 2AM)
  struct tm t3 = makeUTC(2025, 3, 9, 8, 0);
  TEST_ASSERT_EQUAL("Mar 9 08:00 UTC = BEGINS", DST_BEGINS, calculateDSTStatus(&t3, TZ));
  
  // Mar 9, 2025 10:00 UTC = Mar 9 02:00 PST → DST begins (still transition day)
  struct tm t3b = makeUTC(2025, 3, 9, 10, 0);
  TEST_ASSERT_EQUAL("Mar 9 10:00 UTC = BEGINS", DST_BEGINS, calculateDSTStatus(&t3b, TZ));
  
  // Mar 9, 2025 23:00 UTC = Mar 9 15:00 PST (still transition day)
  struct tm t3c = makeUTC(2025, 3, 9, 23, 0);
  TEST_ASSERT_EQUAL("Mar 9 23:00 UTC = BEGINS", DST_BEGINS, calculateDSTStatus(&t3c, TZ));
  
  // ---- Day after spring forward ----
  // Mar 10, 2025 08:00 UTC = Mar 10 00:00 PST = DST in effect
  struct tm t4 = makeUTC(2025, 3, 10, 8, 0);
  TEST_ASSERT_EQUAL("Mar 10 08:00 UTC = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t4, TZ));
  
  // ---- Summer: DST In Effect ----
  // Jul 4, 2025 20:00 UTC = Jul 4 12:00 PDT
  struct tm t5 = makeUTC(2025, 7, 4, 20, 0);
  TEST_ASSERT_EQUAL("Jul 4 20:00 UTC = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t5, TZ));
  
  // ---- Fall Back Day: November 2, 2025 ----
  // Nov 2, 2025 08:00 UTC = Nov 2 00:00 PST (transition day)
  struct tm t6 = makeUTC(2025, 11, 2, 8, 0);
  TEST_ASSERT_EQUAL("Nov 2 08:00 UTC = ENDS", DST_ENDS, calculateDSTStatus(&t6, TZ));
  
  // Nov 2, 2025 20:00 UTC = Nov 2 12:00 PST (still transition day)
  struct tm t6b = makeUTC(2025, 11, 2, 20, 0);
  TEST_ASSERT_EQUAL("Nov 2 20:00 UTC = ENDS", DST_ENDS, calculateDSTStatus(&t6b, TZ));
  
  // ---- Day after fall back ----
  // Nov 3, 2025 08:00 UTC = Nov 3 00:00 PST = Standard
  struct tm t7 = makeUTC(2025, 11, 3, 8, 0);
  TEST_ASSERT_EQUAL("Nov 3 08:00 UTC = STANDARD", DST_STANDARD, calculateDSTStatus(&t7, TZ));
  
  // ---- Late night UTC that rolls to previous local day ----
  // Mar 9, 2025 05:00 UTC = Mar 8 21:00 PST (still Mar 8 locally = not transition day)
  struct tm t8 = makeUTC(2025, 3, 9, 5, 0);
  TEST_ASSERT_EQUAL("Mar 9 05:00 UTC (= Mar 8 local) = STANDARD", DST_STANDARD, calculateDSTStatus(&t8, TZ));
  
  Serial.println();
}

// ============================================================
// Test: Eastern Timezone
// ============================================================
void testCalculateDSTStatus_Eastern() {
  Serial.println("--- DST Status: Eastern (UTC-5) ---");
  
  const int TZ = TZ_US_EASTERN;  // -5
  
  // Jan 15, 2025 17:00 UTC = Jan 15 12:00 EST
  struct tm t1 = makeUTC(2025, 1, 15, 17, 0);
  TEST_ASSERT_EQUAL("Jan 15 17:00 UTC EST = STANDARD", DST_STANDARD, calculateDSTStatus(&t1, TZ));
  
  // Mar 9, 2025 05:00 UTC = Mar 9 00:00 EST (transition day)
  struct tm t2 = makeUTC(2025, 3, 9, 5, 0);
  TEST_ASSERT_EQUAL("Mar 9 05:00 UTC EST = BEGINS", DST_BEGINS, calculateDSTStatus(&t2, TZ));
  
  // Jul 4, 2025 17:00 UTC = Jul 4 12:00 EDT
  struct tm t3 = makeUTC(2025, 7, 4, 17, 0);
  TEST_ASSERT_EQUAL("Jul 4 17:00 UTC EDT = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t3, TZ));
  
  // Nov 2, 2025 05:00 UTC = Nov 2 00:00 EST (transition day)
  struct tm t4 = makeUTC(2025, 11, 2, 5, 0);
  TEST_ASSERT_EQUAL("Nov 2 05:00 UTC EST = ENDS", DST_ENDS, calculateDSTStatus(&t4, TZ));
  
  Serial.println();
}

// ============================================================
// Test: UTC to Local Day Rollback
// ============================================================
void testUTCToLocalRollback() {
  Serial.println("--- UTC to Local Day Rollback ---");
  
  const int TZ = TZ_US_PACIFIC;  // -8
  
  // UTC midnight = previous day local time for negative offsets
  // Jan 1 03:00 UTC = Dec 31 19:00 PST (previous year!)
  struct tm t1 = makeUTC(2025, 1, 1, 3, 0);
  TEST_ASSERT_EQUAL("Jan 1 03:00 UTC (= Dec 31 local) = STANDARD", DST_STANDARD, calculateDSTStatus(&t1, TZ));
  
  // Nov 2 05:00 UTC = Nov 1 21:00 PST (previous day locally = still DST)
  struct tm t2 = makeUTC(2025, 11, 2, 5, 0);
  TEST_ASSERT_EQUAL("Nov 2 05:00 UTC (= Nov 1 local) = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t2, TZ));
  
  // Mar 10 06:00 UTC = Mar 9 22:00 PST (previous day = transition day)
  struct tm t3 = makeUTC(2025, 3, 10, 6, 0);
  TEST_ASSERT_EQUAL("Mar 10 06:00 UTC (= Mar 9 local) = BEGINS", DST_BEGINS, calculateDSTStatus(&t3, TZ));
  
  // Apr 1 07:00 UTC = Mar 31 23:00 PDT (still March locally, DST)
  struct tm t4 = makeUTC(2025, 4, 1, 7, 0);
  TEST_ASSERT_EQUAL("Apr 1 07:00 UTC (= Mar 31 local) = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t4, TZ));
  
  Serial.println();
}

// ============================================================
// Test: Multi-Year DST Transitions
// ============================================================
void testMultiYearTransitions() {
  Serial.println("--- Multi-Year Validation ---");
  
  const int TZ = TZ_US_PACIFIC;
  
  // Validate 2024-2030 transitions
  struct { int year; int startDay; int endDay; } transitions[] = {
    {2024, 10, 3},
    {2025, 9, 2},
    {2026, 8, 1},
    {2027, 14, 7},
    {2028, 12, 5},
    {2029, 11, 4},
    {2030, 10, 3}
  };
  
  for (int i = 0; i < 7; i++) {
    int yr = transitions[i].year;
    int sd = transitions[i].startDay;
    int ed = transitions[i].endDay;
    char name[64];
    
    // Day before spring forward = standard
    sprintf(name, "%d Mar %d = STANDARD", yr, sd - 1);
    struct tm tBefore = makeUTC(yr, 3, sd - 1, 20, 0);
    TEST_ASSERT_EQUAL(name, DST_STANDARD, calculateDSTStatus(&tBefore, TZ));
    
    // Spring forward day = begins
    sprintf(name, "%d Mar %d = BEGINS", yr, sd);
    struct tm tStart = makeUTC(yr, 3, sd, 20, 0);
    TEST_ASSERT_EQUAL(name, DST_BEGINS, calculateDSTStatus(&tStart, TZ));
    
    // Day after spring forward = in effect
    sprintf(name, "%d Mar %d = IN_EFFECT", yr, sd + 1);
    struct tm tAfter = makeUTC(yr, 3, sd + 1, 20, 0);
    TEST_ASSERT_EQUAL(name, DST_IN_EFFECT, calculateDSTStatus(&tAfter, TZ));
    
    // Fall back day = ends
    sprintf(name, "%d Nov %d = ENDS", yr, ed);
    struct tm tEnd = makeUTC(yr, 11, ed, 20, 0);
    TEST_ASSERT_EQUAL(name, DST_ENDS, calculateDSTStatus(&tEnd, TZ));
    
    // Day after fall back = standard
    sprintf(name, "%d Nov %d = STANDARD", yr, ed + 1);
    struct tm tAfterEnd = makeUTC(yr, 11, ed + 1, 20, 0);
    TEST_ASSERT_EQUAL(name, DST_STANDARD, calculateDSTStatus(&tAfterEnd, TZ));
  }
  
  Serial.println();
}

// ============================================================
// Test: Edge Cases
// ============================================================
void testEdgeCases() {
  Serial.println("--- Edge Cases ---");
  
  const int TZ = TZ_US_PACIFIC;
  
  // Midnight UTC on Jan 1 (rolls to Dec 31 previous year locally)
  struct tm t1 = makeUTC(2025, 1, 1, 0, 0);
  TEST_ASSERT_EQUAL("Jan 1 00:00 UTC = STANDARD", DST_STANDARD, calculateDSTStatus(&t1, TZ));
  
  // Last minute of year UTC
  struct tm t2 = makeUTC(2025, 12, 31, 23, 59);
  TEST_ASSERT_EQUAL("Dec 31 23:59 UTC = STANDARD", DST_STANDARD, calculateDSTStatus(&t2, TZ));
  
  // Middle of summer midnight UTC
  struct tm t3 = makeUTC(2025, 7, 15, 0, 0);
  // Jul 15 00:00 UTC = Jul 14 16:00 PDT (still DST)
  TEST_ASSERT_EQUAL("Jul 15 00:00 UTC (= Jul 14 local) = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t3, TZ));
  
  // Leap year Feb 29
  struct tm t4 = makeUTC(2024, 2, 29, 20, 0);
  TEST_ASSERT_EQUAL("Feb 29 2024 = STANDARD", DST_STANDARD, calculateDSTStatus(&t4, TZ));
  
  // Year 2000 (leap year, century divisible by 400)
  struct tm t5 = makeUTC(2000, 7, 4, 20, 0);
  TEST_ASSERT_EQUAL("Jul 4 2000 = IN_EFFECT", DST_IN_EFFECT, calculateDSTStatus(&t5, TZ));
  
  Serial.println();
}
