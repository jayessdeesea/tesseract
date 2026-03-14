/*
 * display_manager.h - 7-Segment Display Manager
 * 
 * Drives 4× Adafruit 0.56" 4-digit 7-segment displays with HT16K33 I2C backpacks.
 * Part: Adafruit #880 (https://www.adafruit.com/product/880)
 * 
 * Display layout:
 *   Display 1 (0x70): Year       e.g., "2026"
 *   Display 2 (0x71): Month.Day  e.g., "03.11"
 *   Display 3 (0x72): Hour:Min   e.g., "14:40" (local time)
 *   Display 4 (0x73): Sec.Status e.g., "34.S"
 * 
 * Wiring (all displays share I2C bus):
 *   ESP32 GPIO 21 (SDA) → All display SDA pins
 *   ESP32 GPIO 22 (SCL) → All display SCL pins
 *   3.3V → All display VCC pins
 *   GND  → All display GND pins
 * 
 * I2C addresses set via solder jumpers A0/A1/A2 on each backpack:
 *   0x70 = no jumpers (default)
 *   0x71 = A0 bridged
 *   0x72 = A1 bridged
 *   0x73 = A0 + A1 bridged
 * 
 * Dependencies:
 *   - Adafruit LED Backpack library (install via Arduino Library Manager)
 *   - Adafruit GFX library (auto-installed as dependency)
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <time.h>

// Status character codes for Display 4
const char DISPLAY_STATUS_STD      = 'S';  // Standard time
const char DISPLAY_STATUS_DST      = 'd';  // DST in effect
const char DISPLAY_STATUS_ERROR    = 'E';  // Error condition
const char DISPLAY_STATUS_CONNECT  = 'C';  // Connecting/syncing

/*
 * Initialize I2C bus and all 4 displays.
 * Call once from setup(). Displays a brief test pattern.
 * 
 * Returns: number of displays found (0-4). Logs warnings for missing displays.
 */
int displayInit();

/*
 * Update all displays with current time and status.
 * Call once per second from main loop.
 * 
 * Parameters:
 *   timeinfo   - Current UTC time
 *   dstActive  - true if DST is in effect
 *   hasError   - true if system is in error state
 */
void displayUpdate(const struct tm* timeinfo, bool dstActive, bool hasError);

/*
 * Show a connecting/startup message on the displays.
 * Call during WiFi/NTP initialization.
 */
void displayShowConnecting();

/*
 * Blank all displays (turn off all segments but keep backlight).
 */
void displayBlank();

/*
 * Set brightness for all displays.
 * 
 * Parameters:
 *   brightness - 0 (dimmest) to 15 (brightest)
 */
void displaySetBrightness(uint8_t brightness);

/*
 * Check if displays are available on the I2C bus.
 * Returns: number of displays responding (0-4).
 */
int displayProbe();

/*
 * Re-scan I2C bus and initialize any newly connected displays.
 * Useful for hot-plug support (displays connected after boot).
 * Returns: total number of displays now available (0-4).
 */
int displayRescan();

/*
 * Set the timezone offset for local time display.
 * The offset is applied to UTC when updating the time display.
 * DST adjustment (+1 hour) is handled automatically via the dstActive flag
 * passed to displayUpdate().
 * 
 * Parameters:
 *   stdOffset - Standard time UTC offset in hours (e.g., -8 for PST)
 */
void displaySetTimezone(int stdOffset);

#endif // DISPLAY_MANAGER_H
