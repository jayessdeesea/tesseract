/*
 * config.h - WWVB Transmitter Configuration
 * 
 * Edit this file with your WiFi credentials and NTP server addresses.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi & NTP Credentials (from separate file, not committed to git)
// ============================================================
// To set up: copy credentials_template.h to credentials.h
// and edit with your real WiFi SSID, password, and NTP server addresses.
#include "credentials.h"

// NTP sync interval in milliseconds (default: 1 hour)
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;

// ============================================================
// GPIO Pin Configuration
// ============================================================
const int WWVB_OUTPUT_PIN = 18;   // PWM output to ULN2003AN driver (pin 1)
const int STATUS_LED_PIN  = 2;    // Built-in LED for basic heartbeat

// Status LED Panel (direct ESP32 drive, 220Ω resistors to GND)
const int LED_NTP_PIN     = 19;   // Green LED: NTP sync status
const int LED_WIFI_PIN    = 23;   // Blue LED: WiFi connection status
const int LED_TX_PIN      = 25;   // Red LED: Transmit activity

// I2C Display (4× Adafruit HT16K33 7-segment backpacks)
const int I2C_SDA_PIN     = 21;   // I2C data
const int I2C_SCL_PIN     = 22;   // I2C clock

// ============================================================
// LEDC PWM Configuration
// ============================================================
const int PWM_FREQ       = 60000;  // 60 kHz carrier frequency
const int PWM_RESOLUTION = 8;     // 8-bit resolution (0-255)
const int PWM_DUTY_ON    = 128;   // 50% duty cycle when carrier on
const int PWM_DUTY_OFF   = 0;     // 0% duty cycle when carrier off

// ============================================================
// WWVB Timing Constants (milliseconds)
// ============================================================
const int BIT_0_LOW_MS    = 200;   // Binary 0: 200ms reduced power
const int BIT_0_HIGH_MS   = 800;   // Binary 0: 800ms full power
const int BIT_1_LOW_MS    = 500;   // Binary 1: 500ms reduced power
const int BIT_1_HIGH_MS   = 500;   // Binary 1: 500ms full power
const int MARKER_LOW_MS   = 800;   // Marker: 800ms reduced power
const int MARKER_HIGH_MS  = 200;   // Marker: 200ms full power

// ============================================================
// WWVB Bit Type Constants
// ============================================================
const uint8_t WWVB_BIT_0  = 0;    // Binary zero
const uint8_t WWVB_BIT_1  = 1;    // Binary one
const uint8_t WWVB_MARKER = 2;    // Position/frame marker

// ============================================================
// Serial Configuration
// ============================================================
const unsigned long SERIAL_BAUD = 115200;

// ============================================================
// Timezone Configuration
// ============================================================
// US timezone UTC offsets (standard time, before DST adjustment)
// DST status is calculated automatically from UTC time + timezone.
// See dst_manager.h for supported timezones.
const int DEFAULT_TIMEZONE = -8;  // US Pacific (Seattle, WA)
                                   // -5 = US Eastern
                                   // -6 = US Central
                                   // -7 = US Mountain
                                   // -8 = US Pacific

// ============================================================
// Timing
// ============================================================
const int WIFI_CONNECT_TIMEOUT_MS = 30000;  // 30 second WiFi timeout
const int NTP_SYNC_TIMEOUT_MS    = 30000;   // 30 second NTP sync timeout

// ============================================================
// Status LED Blink Rates (milliseconds)
// ============================================================
const int LED_BLINK_NO_SYNC    = 250;   // Fast blink when no NTP sync
const int LED_BLINK_WIFI_CONN  = 300;   // Fast blink while WiFi connecting
const int LED_BLINK_NTP_AGING  = 1000;  // Slow blink when NTP aging (>1 hour)
const int LED_TX_FLASH_MS      = 50;    // Brief flash each second for TX activity
const int LED_BRIGHTNESS       = 20;    // 0-255 PWM brightness (20 ≈ 8% duty)

// NTP sync age thresholds (seconds)
const unsigned long NTP_SYNC_FRESH_SEC = 3600;   // <1 hour = solid green
const unsigned long NTP_SYNC_STALE_SEC = 86400;  // >24 hours = LED off

// ============================================================
// 7-Segment Display Configuration (Adafruit HT16K33 Backpacks)
// ============================================================
const uint8_t DISPLAY_ADDR_YEAR   = 0x70;  // Display 1: Year (e.g., 2026)
const uint8_t DISPLAY_ADDR_DATE   = 0x71;  // Display 2: MM.DD (e.g., 03.08)
const uint8_t DISPLAY_ADDR_TIME   = 0x72;  // Display 3: HH:MM UTC (e.g., 12:34)
const uint8_t DISPLAY_ADDR_STATUS = 0x73;  // Display 4: SS.x (e.g., 34.S)
const uint8_t DISPLAY_BRIGHTNESS  = 4;     // 0-15 brightness level
const uint8_t DISPLAY_COUNT       = 4;     // Number of displays

#endif // CONFIG_H
