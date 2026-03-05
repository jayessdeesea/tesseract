/*
 * dst_manager.h - Automatic DST Status Calculator
 * 
 * Calculates WWVB DST status bits automatically based on UTC time
 * and configured US timezone. No manual DST switching needed.
 * 
 * US DST Rules (Energy Policy Act of 2005):
 *   Begins: 2nd Sunday in March at 2:00 AM local time
 *   Ends:   1st Sunday in November at 2:00 AM local time
 * 
 * See specs/dst_calculation.md for complete specification.
 */

#ifndef DST_MANAGER_H
#define DST_MANAGER_H

#include <stdint.h>
#include <time.h>

// DST status values (matches wwvb_encoder.h constants)
// Redefined here to keep module standalone for testing
#ifndef DST_STANDARD
#define DST_STANDARD    0x00  // Standard Time in effect
#define DST_BEGINS      0x02  // DST begins today
#define DST_IN_EFFECT   0x03  // DST in effect
#define DST_ENDS        0x01  // DST ends today
#endif

// US timezone UTC offsets (standard time)
enum USTimezone {
  TZ_US_EASTERN  = -5,   // EST: UTC-5, EDT: UTC-4
  TZ_US_CENTRAL  = -6,   // CST: UTC-6, CDT: UTC-5
  TZ_US_MOUNTAIN = -7,   // MST: UTC-7, MDT: UTC-6
  TZ_US_PACIFIC  = -8    // PST: UTC-8, PDT: UTC-7
};

/*
 * Calculate the WWVB DST status for a given UTC time and timezone.
 * 
 * Returns one of: DST_STANDARD, DST_BEGINS, DST_IN_EFFECT, DST_ENDS
 * 
 * Parameters:
 *   utcTime  - Current UTC time (struct tm*)
 *   timezone - US timezone (enum USTimezone)
 * 
 * Logic:
 *   1. Convert UTC to local time using timezone offset
 *   2. Determine DST transition dates for the current year
 *   3. Check if today is a transition day (BEGINS or ENDS)
 *   4. Otherwise return IN_EFFECT or STANDARD based on date range
 */
uint8_t calculateDSTStatus(const struct tm* utcTime, int timezoneOffset);

/*
 * Get the day-of-month for the Nth occurrence of a weekday in a month.
 * 
 * Parameters:
 *   year    - Full year (e.g., 2025)
 *   month   - Month (1-12)
 *   weekday - Day of week (0=Sunday, 6=Saturday)
 *   n       - Occurrence (1=first, 2=second, etc.)
 * 
 * Returns: Day of month (1-31)
 * 
 * Examples:
 *   nthWeekdayOfMonth(2025, 3, 0, 2) → 9  (2nd Sunday in March 2025)
 *   nthWeekdayOfMonth(2025, 11, 0, 1) → 2  (1st Sunday in November 2025)
 */
int nthWeekdayOfMonth(int year, int month, int weekday, int n);

/*
 * Get the day of week for a given date using Zeller's congruence.
 * Returns 0=Sunday, 1=Monday, ..., 6=Saturday
 */
int dayOfWeek(int year, int month, int day);

/*
 * Get the DST start date (2nd Sunday in March) for a given year.
 * Returns day of month (1-31).
 */
int getDSTStartDay(int year);

/*
 * Get the DST end date (1st Sunday in November) for a given year.
 * Returns day of month (1-31).
 */
int getDSTEndDay(int year);

/*
 * Check if a given local date/time falls within DST.
 * Does NOT account for transition day edge cases.
 * Use calculateDSTStatus() for complete logic.
 * 
 * Parameters:
 *   year, month (1-12), day (1-31), hour (0-23)
 * 
 * Returns: 1 if in DST, 0 if in standard time
 */
int isInDST(int year, int month, int day, int hour);

#endif // DST_MANAGER_H
