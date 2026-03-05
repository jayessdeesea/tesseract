/*
 * debug_utils.h - Debug and Status Utilities
 * 
 * Serial output helpers and LED status patterns for debugging.
 * All functions are inline for header-only convenience.
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>
#include "wwvb_encoder.h"

// LED blink patterns (milliseconds)
#define LED_BLINK_NORMAL   500   // Normal operation: 1Hz blink
#define LED_BLINK_NO_SYNC  100   // Not synced: fast blink
#define LED_BLINK_ERROR    1000  // Error: slow blink

/*
 * Print startup banner to Serial.
 */
inline void printBanner() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("  WWVB 60 kHz Transmitter v1.0");
  Serial.println("  ESP32 + NTP + Ferrite Rod Antenna");
  Serial.println("========================================");
  Serial.println();
}

/*
 * Print current status to Serial.
 */
inline void printStatus(const struct tm* timeinfo, int currentSecond, 
                        uint8_t currentBit, long secSinceSync, int rssi) {
  Serial.printf("[WWVB] %04d-%02d-%02d %02d:%02d:%02d UTC | ",
    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  Serial.printf("Sec:%02d Bit:%c | ", currentSecond,
    currentBit == WWVB_BIT_MARKER ? 'M' : (char)('0' + currentBit));
  
  Serial.printf("NTP:%lds ago | RSSI:%ddBm\n", secSinceSync, rssi);
}

/*
 * Print a compact frame summary showing the bit pattern.
 */
inline void printFrameSummary(const uint8_t* frame, int minute, int hour, 
                               int dayOfYear, int year) {
  Serial.printf("[WWVB] Frame for %02d:%02d UTC, Day %d, Year %d\n",
    hour, minute, dayOfYear, year);
  Serial.print("[WWVB] Bits: ");
  for (int i = 0; i < WWVB_FRAME_SIZE; i++) {
    if (frame[i] == WWVB_BIT_MARKER) {
      Serial.print("M");
    } else {
      Serial.print(frame[i]);
    }
    if ((i + 1) % 10 == 0 && i < 59) Serial.print("|");
  }
  Serial.println();
}

/*
 * Print bit transmission detail (called each second during TX).
 */
inline void printBitDetail(int second, uint8_t bitType) {
  const char* typeStr;
  int lowMs;
  switch (bitType) {
    case WWVB_BIT_ZERO:   typeStr = "0 (200ms)"; lowMs = 200; break;
    case WWVB_BIT_ONE:    typeStr = "1 (500ms)"; lowMs = 500; break;
    case WWVB_BIT_MARKER: typeStr = "M (800ms)"; lowMs = 800; break;
    default:              typeStr = "? (???)";    lowMs = 0;   break;
  }
  Serial.printf("[TX] Sec %02d: %s\n", second, typeStr);
}

#endif // DEBUG_UTILS_H
