/*
 * test_ntp_client.ino - NTP Client Integration Tests
 * 
 * Tests WiFi connection and NTP time synchronization.
 * REQUIRES: WiFi network access and NTP server availability.
 * 
 * Upload to ESP32, open Serial Monitor at 115200 baud.
 * 
 * Edit WiFi credentials and NTP server addresses below before running.
 */

#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>

// ============================================================
// Configuration
// ============================================================
// To set up: copy credentials_template.h to credentials.h
// and edit with your real WiFi SSID, password, and NTP server addresses.
#include "credentials.h"

// ============================================================
// Test Framework
// ============================================================
int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

#define TEST_ASSERT(name, condition) do { \
  totalTests++; \
  if (condition) { \
    passedTests++; \
    Serial.printf("  PASS: %s\n", name); \
  } else { \
    failedTests++; \
    Serial.printf("  FAIL: %s\n", name); \
  } \
} while(0)

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  NTP Client Test Suite");
  Serial.println("========================================");
  Serial.println();
  
  testWiFiConnection();
  testNTPSync();
  testTimeMonotonicity();
  testUTCOffset();
  testTimePrecision();
  testWiFiReconnect();
  
  // Print summary
  Serial.println();
  Serial.println("========================================");
  Serial.printf("  Results: %d/%d PASSED", passedTests, totalTests);
  if (failedTests > 0) {
    Serial.printf(", %d FAILED", failedTests);
  }
  Serial.println();
  Serial.println("========================================");
  Serial.println();
  Serial.println("Continuous time display (verify manually):");
}

void loop() {
  // Continuous time display for manual verification
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.printf("UTC: %04d-%02d-%02d %02d:%02d:%02d (day %d of year)\n",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
      timeinfo.tm_yday + 1);
  }
  delay(5000);
}

// ============================================================
// Test: WiFi Connection
// ============================================================
void testWiFiConnection() {
  Serial.println("--- WiFi Connection ---");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.printf("  Connecting to '%s'...", WIFI_SSID);
  
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  TEST_ASSERT("WiFi connected", WiFi.status() == WL_CONNECTED);
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    TEST_ASSERT("WiFi has IP address", WiFi.localIP() != IPAddress(0, 0, 0, 0));
    TEST_ASSERT("WiFi signal strength > -90dBm", WiFi.RSSI() > -90);
  }
  
  Serial.println();
}

// ============================================================
// Test: NTP Time Sync
// ============================================================
void testNTPSync() {
  Serial.println("--- NTP Time Sync ---");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("  SKIP: WiFi not connected");
    return;
  }
  
  // Configure NTP (UTC, no offset)
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  
  Serial.printf("  NTP servers: %s, %s, %s\n", NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  Serial.print("  Waiting for sync...");
  
  unsigned long startMs = millis();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo, 1000) && millis() - startMs < 15000) {
    Serial.print(".");
  }
  Serial.println();
  
  unsigned long syncTime = millis() - startMs;
  TEST_ASSERT("NTP sync successful", getLocalTime(&timeinfo, 100));
  
  if (getLocalTime(&timeinfo, 100)) {
    Serial.printf("  Sync time: %lu ms\n", syncTime);
    Serial.printf("  Current UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    // Sanity checks
    TEST_ASSERT("Year >= 2025", timeinfo.tm_year + 1900 >= 2025);
    TEST_ASSERT("Month 1-12", timeinfo.tm_mon >= 0 && timeinfo.tm_mon <= 11);
    TEST_ASSERT("Day 1-31", timeinfo.tm_mday >= 1 && timeinfo.tm_mday <= 31);
    TEST_ASSERT("Hour 0-23", timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23);
    TEST_ASSERT("Minute 0-59", timeinfo.tm_min >= 0 && timeinfo.tm_min <= 59);
    TEST_ASSERT("Second 0-59", timeinfo.tm_sec >= 0 && timeinfo.tm_sec <= 59);
    TEST_ASSERT("Sync < 10 seconds", syncTime < 10000);
  }
  
  Serial.println();
}

// ============================================================
// Test: Time Monotonicity
// ============================================================
void testTimeMonotonicity() {
  Serial.println("--- Time Monotonicity ---");
  
  struct tm t1, t2;
  if (!getLocalTime(&t1, 100)) {
    Serial.println("  SKIP: Time not available");
    return;
  }
  
  // Wait 2 seconds and check time advanced
  delay(2000);
  getLocalTime(&t2, 100);
  
  time_t time1 = mktime(&t1);
  time_t time2 = mktime(&t2);
  
  TEST_ASSERT("Time advances (t2 > t1)", time2 > time1);
  TEST_ASSERT("Time advance ~2s (1-4s)", (time2 - time1) >= 1 && (time2 - time1) <= 4);
  
  Serial.printf("  t1: %02d:%02d:%02d, t2: %02d:%02d:%02d, diff: %ld s\n",
    t1.tm_hour, t1.tm_min, t1.tm_sec,
    t2.tm_hour, t2.tm_min, t2.tm_sec,
    (long)(time2 - time1));
  
  Serial.println();
}

// ============================================================
// Test: UTC Offset (Should Be Zero)
// ============================================================
void testUTCOffset() {
  Serial.println("--- UTC Offset Verification ---");
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    Serial.println("  SKIP: Time not available");
    return;
  }
  
  // Get raw epoch time
  time_t now;
  time(&now);
  struct tm utc_tm;
  gmtime_r(&now, &utc_tm);
  
  // Compare getLocalTime vs gmtime - should be identical since offset=0
  TEST_ASSERT("Hour matches gmtime", timeinfo.tm_hour == utc_tm.tm_hour);
  TEST_ASSERT("Minute matches gmtime", timeinfo.tm_min == utc_tm.tm_min);
  
  Serial.printf("  getLocalTime: %02d:%02d:%02d\n", 
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.printf("  gmtime:       %02d:%02d:%02d\n",
    utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
  
  Serial.println();
}

// ============================================================
// Test: Time Precision (Second Boundary Detection)
// ============================================================
void testTimePrecision() {
  Serial.println("--- Time Precision ---");
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    Serial.println("  SKIP: Time not available");
    return;
  }
  
  // Wait for next second boundary
  int startSec = timeinfo.tm_sec;
  unsigned long waitStart = millis();
  
  while (true) {
    getLocalTime(&timeinfo, 10);
    if (timeinfo.tm_sec != startSec) break;
    if (millis() - waitStart > 2000) break;
  }
  
  unsigned long boundaryMs = millis() - waitStart;
  TEST_ASSERT("Second boundary detected < 1.5s", boundaryMs < 1500);
  
  // Measure 10 second boundaries for consistency
  Serial.println("  Measuring 10 second boundaries...");
  unsigned long intervals[10];
  int lastSec = timeinfo.tm_sec;
  unsigned long lastMs = millis();
  
  for (int i = 0; i < 10; i++) {
    while (true) {
      getLocalTime(&timeinfo, 10);
      if (timeinfo.tm_sec != lastSec) {
        unsigned long now = millis();
        intervals[i] = now - lastMs;
        lastMs = now;
        lastSec = timeinfo.tm_sec;
        Serial.printf("  Interval %d: %lu ms\n", i + 1, intervals[i]);
        break;
      }
    }
  }
  
  // Check intervals are approximately 1000ms (±100ms)
  int goodIntervals = 0;
  for (int i = 0; i < 10; i++) {
    if (intervals[i] >= 900 && intervals[i] <= 1100) goodIntervals++;
  }
  TEST_ASSERT("8+ of 10 intervals within ±100ms of 1s", goodIntervals >= 8);
  
  Serial.println();
}

// ============================================================
// Test: WiFi Reconnect
// ============================================================
void testWiFiReconnect() {
  Serial.println("--- WiFi Reconnect ---");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("  SKIP: WiFi not connected initially");
    return;
  }
  
  // Disconnect
  Serial.println("  Disconnecting WiFi...");
  WiFi.disconnect(true);
  delay(2000);
  TEST_ASSERT("WiFi disconnected", WiFi.status() != WL_CONNECTED);
  
  // Reconnect
  Serial.print("  Reconnecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  unsigned long reconnectTime = millis() - startMs;
  TEST_ASSERT("WiFi reconnected", WiFi.status() == WL_CONNECTED);
  Serial.printf("  Reconnect time: %lu ms\n", reconnectTime);
  
  // Verify time still valid after reconnect
  struct tm timeinfo;
  TEST_ASSERT("Time still valid after reconnect", getLocalTime(&timeinfo, 1000));
  
  Serial.println();
}
