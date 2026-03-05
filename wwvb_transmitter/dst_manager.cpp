/*
 * dst_manager.cpp - Automatic DST Status Calculator
 * 
 * Implements US DST rules per Energy Policy Act of 2005:
 *   Begins: 2nd Sunday in March at 2:00 AM local time
 *   Ends:   1st Sunday in November at 2:00 AM local time
 * 
 * All calculations are pure arithmetic — no timezone libraries needed.
 */

#include "dst_manager.h"

// Days in each month (non-leap year)
static const int MONTH_DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 * Day of week using Tomohiko Sakamoto's algorithm.
 * Returns 0=Sunday, 1=Monday, ..., 6=Saturday.
 * Valid for any Gregorian date.
 */
int dayOfWeek(int year, int month, int day) {
  // Sakamoto's algorithm
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
  // Find day of week for the 1st of the month
  int dow1 = dayOfWeek(year, month, 1);
  
  // Calculate first occurrence of the target weekday
  int firstOccurrence = 1 + ((weekday - dow1 + 7) % 7);
  
  // Calculate Nth occurrence
  int day = firstOccurrence + (n - 1) * 7;
  
  return day;
}

/*
 * DST start: 2nd Sunday in March
 */
int getDSTStartDay(int year) {
  return nthWeekdayOfMonth(year, 3, 0, 2);  // 2nd Sunday in March
}

/*
 * DST end: 1st Sunday in November
 */
int getDSTEndDay(int year) {
  return nthWeekdayOfMonth(year, 11, 0, 1);  // 1st Sunday in November
}

/*
 * Check if a local date/time falls within DST period.
 * Handles the transition hours on boundary days.
 * 
 * month: 1-12, day: 1-31, hour: 0-23 (local standard time hour)
 */
int isInDST(int year, int month, int day, int hour) {
  int dstStartDay = getDSTStartDay(year);
  int dstEndDay = getDSTEndDay(year);
  
  // Months clearly in standard time: Jan, Feb, Dec
  if (month < 3 || month > 11) return 0;
  
  // Months clearly in DST: Apr through Oct
  if (month > 3 && month < 11) return 1;
  
  // March: DST starts on dstStartDay at 2:00 AM local
  if (month == 3) {
    if (day < dstStartDay) return 0;
    if (day > dstStartDay) return 1;
    // Transition day: DST begins at 2:00 AM local standard time
    return (hour >= 2) ? 1 : 0;
  }
  
  // November: DST ends on dstEndDay at 2:00 AM local DST time (= 1:00 AM standard)
  if (month == 11) {
    if (day < dstEndDay) return 1;
    if (day > dstEndDay) return 0;
    // Transition day: DST ends at 2:00 AM DST = 1:00 AM standard
    // Before 1:00 AM standard = still DST
    return (hour < 1) ? 1 : 0;
  }
  
  return 0;  // Should not reach here
}

/*
 * Calculate the WWVB DST status bits from UTC time and timezone.
 * 
 * The WWVB signal uses 4 states:
 *   DST_STANDARD  (00) - Standard time in effect
 *   DST_BEGINS    (10) - DST begins today (transition day)
 *   DST_IN_EFFECT (11) - DST in effect
 *   DST_ENDS      (01) - DST ends today (transition day)
 * 
 * WWVB convention: "today" for BEGINS/ENDS refers to the LOCAL date.
 * The transition happens at 2:00 AM local time.
 * 
 * Strategy:
 *   1. Convert UTC to local STANDARD time (always use standard offset)
 *   2. Determine if today is a transition day
 *   3. If transition day, return BEGINS or ENDS
 *   4. Otherwise, return IN_EFFECT or STANDARD based on current DST state
 */
uint8_t calculateDSTStatus(const struct tm* utcTime, int timezoneOffset) {
  // Convert UTC to local standard time
  // We work in standard time to avoid the ambiguity of "spring forward"
  int localHour = utcTime->tm_hour + timezoneOffset;
  int localDay = utcTime->tm_mday;
  int localMonth = utcTime->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int localYear = utcTime->tm_year + 1900;
  
  // Handle day rollback (UTC hour + negative offset could be previous day)
  if (localHour < 0) {
    localHour += 24;
    localDay--;
    if (localDay < 1) {
      localMonth--;
      if (localMonth < 1) {
        localMonth = 12;
        localYear--;
      }
      // Get days in previous month
      int prevMonthIdx = localMonth - 1;  // 0-indexed for array
      localDay = MONTH_DAYS[prevMonthIdx];
      if (prevMonthIdx == 1 && ((localYear % 4 == 0 && localYear % 100 != 0) || localYear % 400 == 0)) {
        localDay = 29;  // February in leap year
      }
    }
  }
  
  // Get DST transition dates for this year
  int dstStartDay = getDSTStartDay(localYear);
  int dstEndDay = getDSTEndDay(localYear);
  
  // Check if today is a DST transition day
  if (localMonth == 3 && localDay == dstStartDay) {
    return DST_BEGINS;
  }
  
  if (localMonth == 11 && localDay == dstEndDay) {
    return DST_ENDS;
  }
  
  // Not a transition day — determine current DST state
  if (isInDST(localYear, localMonth, localDay, localHour)) {
    return DST_IN_EFFECT;
  }
  
  return DST_STANDARD;
}
