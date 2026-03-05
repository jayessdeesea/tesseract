/*
 * config.h - WWVB Transmitter Configuration
 * 
 * Edit this file with your WiFi credentials and NTP server addresses.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi Configuration
// ============================================================
const char* WIFI_SSID     = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

// ============================================================
// NTP Server Configuration (priority order)
// ============================================================
const char* NTP_SERVER_1 = "192.168.1.100";   // Primary: local stratum-1
const char* NTP_SERVER_2 = "192.168.1.101";   // Secondary: local stratum-1
const char* NTP_SERVER_3 = "pool.ntp.org";     // Tertiary: internet fallback

// NTP sync interval in milliseconds (default: 1 hour)
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;

// ============================================================
// GPIO Pin Configuration
// ============================================================
const int WWVB_OUTPUT_PIN = 18;   // PWM output to transistor driver
const int STATUS_LED_PIN  = 2;    // Built-in LED for status indication

// ============================================================
// LEDC PWM Configuration
// ============================================================
const int PWM_CHANNEL    = 0;      // LEDC channel (0-15)
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
const int LED_BLINK_NO_SYNC = 250;  // Fast blink when no NTP sync

#endif // CONFIG_H
