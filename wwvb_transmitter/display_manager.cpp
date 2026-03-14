/*
 * display_manager.cpp - 7-Segment Display Manager Implementation
 * 
 * Uses Adafruit LED Backpack library to drive HT16K33-based displays.
 * 
 * Digit positions on each 4-digit display:
 *   Position 0 = leftmost digit
 *   Position 1 = second digit
 *   Position 2 = colon (not a digit — use drawColon())
 *   Position 3 = third digit
 *   Position 4 = rightmost digit
 */

#include "display_manager.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

// Display instances
static Adafruit_7segment dispYear;
static Adafruit_7segment dispDate;
static Adafruit_7segment dispTime;
static Adafruit_7segment dispStatus;

// Track which displays are available
static bool dispAvailable[DISPLAY_COUNT] = {false, false, false, false};
static int dispFoundCount = 0;

// Timezone offset for local time display (default: UTC)
static int timezoneStdOffset = 0;

// Previous display values for smart update (anti-flicker)
static int prevYear = -1, prevMonth = -1, prevDay = -1;
static int prevHour = -1, prevMin = -1, prevSec = -1;
static char prevStatusChar = 0;

// Display addresses in order
static const uint8_t dispAddrs[DISPLAY_COUNT] = {
  DISPLAY_ADDR_YEAR,
  DISPLAY_ADDR_DATE,
  DISPLAY_ADDR_TIME,
  DISPLAY_ADDR_STATUS
};

// Display instance pointers for iteration
static Adafruit_7segment* displays[DISPLAY_COUNT] = {
  &dispYear, &dispDate, &dispTime, &dispStatus
};

static const char* dispNames[DISPLAY_COUNT] = {
  "Year", "Date", "Time", "Status"
};


/*
 * Write a 2-digit number to positions on a display.
 * pos0 and pos1 are the digit positions (skipping position 2 which is the colon).
 */
static void writeTwoDigits(Adafruit_7segment* disp, int pos0, int pos1, 
                           int value, bool dot) {
  disp->writeDigitNum(pos0, value / 10);
  disp->writeDigitNum(pos1, value % 10, dot);
}


int displayInit() {
  // Initialize I2C with explicit pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  Serial.println("[DISP] Initializing 7-segment displays...");
  
  dispFoundCount = 0;
  
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (displays[i]->begin(dispAddrs[i])) {
      dispAvailable[i] = true;
      dispFoundCount++;
      displays[i]->setBrightness(DISPLAY_BRIGHTNESS);
      displays[i]->clear();
      displays[i]->writeDisplay();
      Serial.printf("[DISP] %s display (0x%02X): OK\n", dispNames[i], dispAddrs[i]);
    } else {
      dispAvailable[i] = false;
      Serial.printf("[DISP] %s display (0x%02X): NOT FOUND\n", dispNames[i], dispAddrs[i]);
    }
  }
  
  Serial.printf("[DISP] %d of %d displays found\n", dispFoundCount, DISPLAY_COUNT);
  
  // Brief test pattern: show "----" on all available displays
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (dispAvailable[i]) {
      displays[i]->writeDigitRaw(0, 0x40);  // dash segment
      displays[i]->writeDigitRaw(1, 0x40);
      displays[i]->writeDigitRaw(3, 0x40);
      displays[i]->writeDigitRaw(4, 0x40);
      displays[i]->writeDisplay();
    }
  }
  delay(500);
  
  return dispFoundCount;
}


void displayUpdate(const struct tm* timeinfo, bool dstActive, bool hasError) {
  // Convert UTC to local time for display
  int year  = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;
  int day   = timeinfo->tm_mday;
  int hour  = timeinfo->tm_hour;
  int min   = timeinfo->tm_min;
  int sec   = timeinfo->tm_sec;
  
  // Apply timezone offset (standard + DST adjustment)
  if (timezoneStdOffset != 0) {
    int offset = timezoneStdOffset + (dstActive ? 1 : 0);
    
    // Use mktime/localtime for correct date rollover
    struct tm local = *timeinfo;
    time_t utcEpoch = mktime(&local);
    utcEpoch += (long)offset * 3600L;
    struct tm* localTime = gmtime(&utcEpoch);
    
    year  = localTime->tm_year + 1900;
    month = localTime->tm_mon + 1;
    day   = localTime->tm_mday;
    hour  = localTime->tm_hour;
    min   = localTime->tm_min;
    sec   = localTime->tm_sec;
  }
  
  // Determine status character
  char statusChar;
  if (hasError) {
    statusChar = DISPLAY_STATUS_ERROR;
  } else if (dstActive) {
    statusChar = DISPLAY_STATUS_DST;
  } else {
    statusChar = DISPLAY_STATUS_STD;
  }
  
  // ---- Display 1: Year (e.g., "2026") — only update when changed ----
  if (dispAvailable[0] && year != prevYear) {
    dispYear.writeDigitNum(0, year / 1000);
    dispYear.writeDigitNum(1, (year / 100) % 10);
    dispYear.writeDigitNum(3, (year / 10) % 10);
    dispYear.writeDigitNum(4, year % 10);
    dispYear.drawColon(false);
    dispYear.writeDisplay();
    prevYear = year;
  }
  
  // ---- Display 2: Month.Day (e.g., "03.11") — only update when changed ----
  if (dispAvailable[1] && (month != prevMonth || day != prevDay)) {
    writeTwoDigits(&dispDate, 0, 1, month, true);  // MM. (dot after month)
    writeTwoDigits(&dispDate, 3, 4, day, false);    // DD
    dispDate.drawColon(false);
    dispDate.writeDisplay();
    prevMonth = month;
    prevDay = day;
  }
  
  // ---- Display 3: Hour:Min (e.g., "14:40") — only update when changed ----
  if (dispAvailable[2] && (hour != prevHour || min != prevMin)) {
    writeTwoDigits(&dispTime, 0, 1, hour, false);
    writeTwoDigits(&dispTime, 3, 4, min, false);
    dispTime.drawColon(true);  // Colon between HH:MM
    dispTime.writeDisplay();
    prevHour = hour;
    prevMin = min;
  }
  
  // ---- Display 4: Seconds + Status (e.g., "34.S") — updates every second ----
  if (dispAvailable[3] && (sec != prevSec || statusChar != prevStatusChar)) {
    writeTwoDigits(&dispStatus, 0, 1, sec, true);  // SS. (dot after seconds)
    dispStatus.writeDigitAscii(3, statusChar);
    dispStatus.writeDigitRaw(4, 0x00);  // Position 4 blank
    dispStatus.drawColon(false);
    dispStatus.writeDisplay();
    prevSec = sec;
    prevStatusChar = statusChar;
  }
}


void displayShowConnecting() {
  // Show "Conn" across the status display, dashes on others
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (!dispAvailable[i]) continue;
    
    if (i == 3) {
      // Status display: show "Conn"
      dispStatus.writeDigitAscii(0, 'C');
      dispStatus.writeDigitAscii(1, 'o');
      dispStatus.writeDigitAscii(3, 'n');
      dispStatus.writeDigitAscii(4, 'n');
      dispStatus.drawColon(false);
      dispStatus.writeDisplay();
    } else {
      // Other displays: show "----"
      displays[i]->writeDigitRaw(0, 0x40);
      displays[i]->writeDigitRaw(1, 0x40);
      displays[i]->writeDigitRaw(3, 0x40);
      displays[i]->writeDigitRaw(4, 0x40);
      displays[i]->drawColon(false);
      displays[i]->writeDisplay();
    }
  }
}


void displayBlank() {
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (dispAvailable[i]) {
      displays[i]->clear();
      displays[i]->writeDisplay();
    }
  }
}


void displaySetBrightness(uint8_t brightness) {
  if (brightness > 15) brightness = 15;
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (dispAvailable[i]) {
      displays[i]->setBrightness(brightness);
    }
  }
}


int displayProbe() {
  int found = 0;
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    Wire.beginTransmission(dispAddrs[i]);
    if (Wire.endTransmission() == 0) {
      found++;
    }
  }
  return found;
}


int displayRescan() {
  int newFound = 0;
  
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (dispAvailable[i]) {
      newFound++;
      continue;  // Already initialized, skip
    }
    
    // Try to initialize this display
    if (displays[i]->begin(dispAddrs[i])) {
      dispAvailable[i] = true;
      newFound++;
      displays[i]->setBrightness(DISPLAY_BRIGHTNESS);
      displays[i]->clear();
      displays[i]->writeDisplay();
      Serial.printf("[DISP] %s display (0x%02X): HOT-PLUG detected\n", 
                    dispNames[i], dispAddrs[i]);
    }
  }
  
  if (newFound > dispFoundCount) {
    Serial.printf("[DISP] Rescan: %d of %d displays now available\n", 
                  newFound, DISPLAY_COUNT);
    // Reset previous values to force full redraw on newly connected displays
    prevYear = -1; prevMonth = -1; prevDay = -1;
    prevHour = -1; prevMin = -1; prevSec = -1;
    prevStatusChar = 0;
  }
  
  dispFoundCount = newFound;
  return dispFoundCount;
}


void displaySetTimezone(int stdOffset) {
  timezoneStdOffset = stdOffset;
  // Reset cached values to force immediate redraw with new timezone
  prevYear = -1; prevMonth = -1; prevDay = -1;
  prevHour = -1; prevMin = -1; prevSec = -1;
  Serial.printf("[DISP] Timezone set to UTC%+d (local time display)\n", stdOffset);
}
