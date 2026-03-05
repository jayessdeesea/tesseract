/*
 * ntp_manager.h - NTP Time Synchronization Manager
 * 
 * Manages WiFi connection and NTP time sync with multi-server failover.
 * Uses ESP32's built-in SNTP client (lwIP stack).
 * 
 * See specs/ntp_integration.md for design details.
 */

#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <time.h>

// Sync status
enum NtpStatus {
  NTP_NOT_SYNCED,      // No time sync yet
  NTP_SYNCED,          // Time synchronized successfully
  NTP_WIFI_DISCONNECTED // WiFi not connected
};

/*
 * Initialize WiFi connection and NTP sync.
 * Blocks until WiFi connected and time synced, or timeout.
 * 
 * Parameters:
 *   ssid      - WiFi network name
 *   password  - WiFi password
 *   server1   - Primary NTP server
 *   server2   - Secondary NTP server  
 *   server3   - Tertiary NTP server
 * 
 * Returns: true if time synced successfully, false on timeout.
 */
bool ntpInit(const char* ssid, const char* password,
             const char* server1, const char* server2, const char* server3);

/*
 * Get current UTC time.
 * 
 * Parameters:
 *   timeinfo - Output struct tm (filled with UTC time)
 * 
 * Returns: true if time is valid, false if not synced.
 */
bool ntpGetTime(struct tm* timeinfo);

/*
 * Get current NTP sync status.
 */
NtpStatus ntpGetStatus();

/*
 * Check WiFi connection and reconnect if needed.
 * Should be called periodically from main loop.
 */
void ntpMaintain();

/*
 * Get time since last successful NTP sync (seconds).
 * Returns -1 if never synced.
 */
long ntpGetSecondsSinceSync();

/*
 * Get WiFi signal strength (RSSI in dBm).
 * Returns 0 if not connected.
 */
int ntpGetRSSI();

#endif // NTP_MANAGER_H
