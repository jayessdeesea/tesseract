/*
 * credentials_template.h - WiFi & NTP Credential Template
 * 
 * SETUP:
 *   1. Copy this file to credentials.h (in this same directory)
 *   2. Edit credentials.h with your real WiFi and NTP settings
 *   3. credentials.h is in .gitignore — it will never be committed
 * 
 * DO NOT edit this template with real credentials.
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

const char* const WIFI_SSID     = "your-network-name";
const char* const WIFI_PASSWORD = "your-wifi-password";

const char* const NTP_SERVER_1  = "192.168.1.100";   // Primary: local stratum-1
const char* const NTP_SERVER_2  = "192.168.1.101";   // Secondary: local stratum-1
const char* const NTP_SERVER_3  = "pool.ntp.org";     // Tertiary: internet fallback

#endif // CREDENTIALS_H
