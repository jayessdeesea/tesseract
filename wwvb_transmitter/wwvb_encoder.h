/*
 * wwvb_encoder.h - WWVB Time Code Encoder
 * 
 * Encodes UTC time into a 60-bit WWVB amplitude-modulated time code frame.
 * Pure logic module - no hardware dependencies.
 * 
 * See specs/wwvb_protocol.md for complete protocol specification.
 */

#ifndef WWVB_ENCODER_H
#define WWVB_ENCODER_H

#include <stdint.h>
#include <time.h>

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

/*
 * Encode a complete WWVB 60-bit frame for the given UTC time.
 * 
 * IMPORTANT: The time passed in should be the NEXT minute's time.
 * WWVB transmits the time for the upcoming minute during the current minute.
 * 
 * Parameters:
 *   timeinfo  - UTC time for the next minute (struct tm*)
 *   frame     - Output array of 60 uint8_t values (0, 1, or 2=marker)
 *   dstStatus - DST status (use DST_* constants)
 */
void encodeWWVBFrame(const struct tm* timeinfo, uint8_t* frame, uint8_t dstStatus);

/*
 * Calculate the day of year (1-366) from a struct tm.
 * Uses tm_mon (0-11) and tm_mday (1-31) and tm_year.
 * Returns 1-366.
 */
int calculateDayOfYear(const struct tm* timeinfo);

/*
 * Determine if the given year is a leap year.
 * Year is the full year (e.g., 2025).
 * Returns 1 if leap year, 0 otherwise.
 */
int isLeapYear(int year);

/*
 * Advance a struct tm by one minute.
 * Handles all rollovers: minute→hour→day→month→year.
 * Modifies the struct in place.
 */
void advanceOneMinute(struct tm* timeinfo);

/*
 * Get the low-power duration in milliseconds for a given bit type.
 * Returns 200 for BIT_ZERO, 500 for BIT_ONE, 800 for MARKER.
 */
int getLowDurationMs(uint8_t bitType);

/*
 * Get the full-power duration in milliseconds for a given bit type.
 * Returns 800 for BIT_ZERO, 500 for BIT_ONE, 200 for MARKER.
 */
int getHighDurationMs(uint8_t bitType);

/*
 * Print a WWVB frame to Serial in human-readable format.
 * Requires Serial to be initialized.
 */
void printFrame(const uint8_t* frame);

#endif // WWVB_ENCODER_H
