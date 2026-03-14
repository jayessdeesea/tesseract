/*
 * ntp_manager.cpp - NTP Time Synchronization Manager Implementation
 * 
 * Uses ESP32's built-in SNTP client via configTime().
 * WiFi auto-reconnect is enabled by default on ESP32.
 */

#include "ntp_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

// Module state
static NtpStatus currentStatus = NTP_NOT_SYNCED;
static unsigned long lastSyncMillis = 0;
static bool timeSynced = false;

// SNTP sync callback
static void timeSyncCallback(struct timeval *tv) {
  timeSynced = true;
  lastSyncMillis = millis();
  currentStatus = NTP_SYNCED;
  Serial.println("[NTP] Time synchronized successfully");
  
  struct tm timeinfo;
  gmtime_r(&tv->tv_sec, &timeinfo);
  Serial.printf("[NTP] UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}


bool ntpInit(const char* ssid, const char* password,
             const char* server1, const char* server2, const char* server3) {
  
  Serial.println("[NTP] Connecting to WiFi...");
  Serial.printf("[NTP] SSID: %s\n", ssid);
  
  // Set hostname to "tesseract-XXXXXX" (last 6 hex digits of MAC address)
  WiFi.mode(WIFI_STA);
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String hostname = "tesseract-" + mac.substring(6);
  WiFi.setHostname(hostname.c_str());
  Serial.printf("[NTP] Hostname: %s\n", hostname.c_str());
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startMs > 30000) {  // 30 second timeout
      Serial.println("\n[NTP] WiFi connection timeout!");
      currentStatus = NTP_WIFI_DISCONNECTED;
      return false;
    }
  }
  
  Serial.printf("\n[NTP] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[NTP] RSSI: %d dBm\n", WiFi.RSSI());
  
  // Register sync callback
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  
  // Configure NTP with UTC (no offset)
  configTime(0, 0, server1, server2, server3);
  
  Serial.printf("[NTP] Servers: %s, %s, %s\n", server1, server2, server3);
  Serial.println("[NTP] Waiting for time sync...");
  
  // Wait for initial sync
  startMs = millis();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo, 1000)) {
    Serial.print(".");
    if (millis() - startMs > 30000) {  // 30 second timeout
      Serial.println("\n[NTP] Time sync timeout!");
      currentStatus = NTP_NOT_SYNCED;
      return false;
    }
  }
  
  // Mark as synced even if callback hasn't fired yet
  timeSynced = true;
  lastSyncMillis = millis();
  currentStatus = NTP_SYNCED;
  
  Serial.printf("\n[NTP] Initial sync complete: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  return true;
}


bool ntpGetTime(struct tm* timeinfo) {
  if (!timeSynced) return false;
  
  // getLocalTime returns UTC since we configured offset=0
  if (!getLocalTime(timeinfo, 100)) {
    return false;
  }
  
  return true;
}


NtpStatus ntpGetStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    return NTP_WIFI_DISCONNECTED;
  }
  return currentStatus;
}


void ntpMaintain() {
  // Check WiFi status and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NTP] WiFi disconnected, attempting reconnect...");
    WiFi.reconnect();
    
    // Brief wait for reconnect
    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 10000) {
      delay(500);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[NTP] WiFi reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[NTP] WiFi reconnect failed, will retry later");
    }
  }
}


long ntpGetSecondsSinceSync() {
  if (!timeSynced) return -1;
  return (millis() - lastSyncMillis) / 1000;
}


int ntpGetRSSI() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  return WiFi.RSSI();
}
