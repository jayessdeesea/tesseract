/*
 * wwvb_encoder.cpp - WWVB Time Code Encoder Implementation
 * 
 * Encodes UTC time into a 60-bit WWVB AM time code frame per NIST spec.
 * 
 * Frame layout reference: specs/wwvb_protocol.md
 * 
 * Key implementation notes:
 * - Time passed in must be for the NEXT minute (WWVB convention)
 * - All time values are UTC
 * - BCD encoding, most significant weight first
 * - UT1 correction hardcoded to +0.0 (irrelevant for consumer clocks)
 * - Leap second warning hardcoded to 0
 */

#include "wwvb_encoder.h"
#include <Arduino.h>
#include <string.h>

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
