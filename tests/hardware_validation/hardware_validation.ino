/*
 * hardware_validation.ino - Full System Integration Test
 * 
 * Complete hardware validation: NTP sync → WWVB encode → transmit.
 * Tests the entire signal chain with real hardware.
 * 
 * REQUIRES:
 *   - WiFi network with NTP server access
 *   - Oscilloscope on GPIO 18 (optional but recommended)
 *   - Transistor driver circuit connected (optional)
 *   - WWVB clock nearby for reception test (optional)
 * 
 * Upload to ESP32, open Serial Monitor at 115200 baud.
 * 
 * This test runs continuously, transmitting valid WWVB frames.
 * It's essentially the full transmitter with extra diagnostics.
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

// Hardware pins
const int OUTPUT_PIN     = 18;
const int LED_PIN        = 2;
const int PWM_FREQ       = 60000;
const int PWM_RESOLUTION = 8;
const int PWM_DUTY_ON    = 128;
const int PWM_DUTY_OFF   = 0;

// DST status - set appropriately for your timezone
const uint8_t DST_STATUS = 0x03;  // DST in effect (change as needed)

// ============================================================
// WWVB Encoder (embedded for standalone test)
// ============================================================
#define WWVB_BIT_ZERO   0
#define WWVB_BIT_ONE    1
#define WWVB_BIT_MARKER 2

static const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int isLeapYear(int year) {
  if (year % 400 == 0) return 1;
  if (year % 100 == 0) return 0;
  if (year % 4 == 0) return 1;
  return 0;
}

int calculateDayOfYear(const struct tm* t) {
  int year = t->tm_year + 1900;
  int doy = 0;
  for (int m = 0; m < t->tm_mon; m++) {
    doy += DAYS_IN_MONTH[m];
    if (m == 1 && isLeapYear(year)) doy++;
  }
  return doy + t->tm_mday;
}

void advanceOneMinute(struct tm* t) {
  t->tm_min++;
  if (t->tm_min >= 60) {
    t->tm_min = 0;
    t->tm_hour++;
    if (t->tm_hour >= 24) {
      t->tm_hour = 0;
      t->tm_mday++;
      int year = t->tm_year + 1900;
      int dim = DAYS_IN_MONTH[t->tm_mon];
      if (t->tm_mon == 1 && isLeapYear(year)) dim = 29;
      if (t->tm_mday > dim) {
        t->tm_mday = 1;
        t->tm_mon++;
        if (t->tm_mon >= 12) { t->tm_mon = 0; t->tm_year++; }
      }
    }
  }
}

void encodeWWVBFrame(const struct tm* t, uint8_t* frame, uint8_t dstStatus) {
  memset(frame, 0, 60);
  
  int minute = t->tm_min;
  int hour = t->tm_hour;
  int doy = calculateDayOfYear(t);
  int year = (t->tm_year + 1900) % 100;
  int fullYear = t->tm_year + 1900;
  
  // Markers
  int markers[] = {0, 9, 19, 29, 39, 49, 59};
  for (int i = 0; i < 7; i++) frame[markers[i]] = WWVB_BIT_MARKER;
  
  // Minutes
  frame[1] = (minute >= 40) ? 1 : 0;
  frame[2] = ((minute % 40) >= 20) ? 1 : 0;
  frame[3] = ((minute % 20) >= 10) ? 1 : 0;
  int mo = minute % 10;
  frame[5] = (mo >= 8) ? 1 : 0;
  frame[6] = ((mo % 8) >= 4) ? 1 : 0;
  frame[7] = ((mo % 4) >= 2) ? 1 : 0;
  frame[8] = (mo % 2) ? 1 : 0;
  
  // Hours
  frame[12] = (hour >= 20) ? 1 : 0;
  frame[13] = ((hour % 20) >= 10) ? 1 : 0;
  int ho = hour % 10;
  frame[15] = (ho >= 8) ? 1 : 0;
  frame[16] = ((ho % 8) >= 4) ? 1 : 0;
  frame[17] = ((ho % 4) >= 2) ? 1 : 0;
  frame[18] = (ho % 2) ? 1 : 0;
  
  // Day of year
  frame[22] = (doy >= 200) ? 1 : 0;
  frame[23] = ((doy % 200) >= 100) ? 1 : 0;
  int dt = doy % 100;
  frame[25] = (dt >= 80) ? 1 : 0;
  frame[26] = ((dt % 80) >= 40) ? 1 : 0;
  frame[27] = ((dt % 40) >= 20) ? 1 : 0;
  frame[28] = ((dt % 20) >= 10) ? 1 : 0;
  int d1 = doy % 10;
  frame[30] = (d1 >= 8) ? 1 : 0;
  frame[31] = ((d1 % 8) >= 4) ? 1 : 0;
  frame[32] = ((d1 % 4) >= 2) ? 1 : 0;
  frame[33] = (d1 % 2) ? 1 : 0;
  
  // UT1 sign (positive)
  frame[36] = 1; frame[37] = 0; frame[38] = 1;
  
  // Year
  int yt = year / 10;
  frame[45] = (yt >= 8) ? 1 : 0;
  frame[46] = ((yt % 8) >= 4) ? 1 : 0;
  frame[47] = ((yt % 4) >= 2) ? 1 : 0;
  frame[48] = (yt % 2) ? 1 : 0;
  int y1 = year % 10;
  frame[50] = (y1 >= 8) ? 1 : 0;
  frame[51] = ((y1 % 8) >= 4) ? 1 : 0;
  frame[52] = ((y1 % 4) >= 2) ? 1 : 0;
  frame[53] = (y1 % 2) ? 1 : 0;
  
  // Flags
  frame[55] = isLeapYear(fullYear) ? 1 : 0;
  frame[56] = 0;  // No leap second
  frame[57] = (dstStatus & 0x02) ? 1 : 0;
  frame[58] = (dstStatus & 0x01) ? 1 : 0;
}

// ============================================================
// State
// ============================================================
uint8_t wwvbFrame[60];
int lastSecond = -1;
bool frameReady = false;
unsigned long frameCount = 0;
unsigned long startMillis = 0;
unsigned long totalBitsTransmitted = 0;

// Diagnostics
unsigned long minBitTime = 99999;
unsigned long maxBitTime = 0;

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("================================================");
  Serial.println("  WWVB Hardware Validation Test");
  Serial.println("  Full System Integration");
  Serial.println("================================================");
  Serial.println();
  
  // PWM setup (ESP32 Core 3.x API)
  ledcAttach(OUTPUT_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("[HW] PWM configured: 60 kHz on GPIO 18");
  
  // WiFi + NTP
  Serial.printf("[HW] Connecting to WiFi '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[HW] FAIL: WiFi connection failed!");
    Serial.println("[HW] Check SSID/password and try again.");
    while (1) { delay(1000); }
  }
  
  Serial.printf("\n[HW] WiFi OK! IP: %s, RSSI: %d dBm\n",
    WiFi.localIP().toString().c_str(), WiFi.RSSI());
  
  // NTP sync
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  Serial.println("[HW] Waiting for NTP sync...");
  
  struct tm timeinfo;
  unsigned long ntpStart = millis();
  while (!getLocalTime(&timeinfo, 1000) && millis() - ntpStart < 30000) {
    Serial.print(".");
  }
  
  if (!getLocalTime(&timeinfo, 100)) {
    Serial.println("\n[HW] FAIL: NTP sync failed!");
    while (1) { delay(1000); }
  }
  
  Serial.printf("\n[HW] NTP OK! UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  startMillis = millis();
  
  Serial.println();
  Serial.println("================================================");
  Serial.println("  SYSTEM READY - Transmitting WWVB frames");
  Serial.println("  Place WWVB clock near antenna to test sync");
  Serial.println("================================================");
  Serial.println();
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
  struct tm timeinfo;
  
  if (!getLocalTime(&timeinfo, 100)) {
    Serial.println("[HW] WARNING: Time not available");
    delay(1000);
    return;
  }
  
  int currentSecond = timeinfo.tm_sec;
  
  if (currentSecond != lastSecond) {
    lastSecond = currentSecond;
    
    // Encode new frame at second 0
    if (currentSecond == 0) {
      struct tm nextMin = timeinfo;
      advanceOneMinute(&nextMin);
      encodeWWVBFrame(&nextMin, wwvbFrame, DST_STATUS);
      frameReady = true;
      frameCount++;
      
      int doy = calculateDayOfYear(&nextMin);
      Serial.printf("\n[FRAME %lu] Encoding: %04d-%02d-%02d %02d:%02d UTC (Day %d)\n",
        frameCount, nextMin.tm_year + 1900, nextMin.tm_mon + 1, nextMin.tm_mday,
        nextMin.tm_hour, nextMin.tm_min, doy);
      
      // Print frame
      Serial.print("[FRAME] ");
      for (int i = 0; i < 60; i++) {
        Serial.print(wwvbFrame[i] == 2 ? "M" : String(wwvbFrame[i]));
        if ((i + 1) % 10 == 0 && i < 59) Serial.print("|");
      }
      Serial.println();
    }
    
    // Transmit bit
    if (frameReady) {
      uint8_t bit = wwvbFrame[currentSecond];
      int lowMs = (bit == 0) ? 200 : (bit == 1) ? 500 : 800;
      
      unsigned long bitStart = millis();
      
      // Carrier OFF
      ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
      digitalWrite(LED_PIN, LOW);
      delay(lowMs);
      
      // Carrier ON
      ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
      digitalWrite(LED_PIN, HIGH);
      
      unsigned long bitDuration = millis() - bitStart;
      totalBitsTransmitted++;
      
      // Track timing accuracy
      if (bitDuration < minBitTime) minBitTime = bitDuration;
      if (bitDuration > maxBitTime) maxBitTime = bitDuration;
      
      // Print every bit
      const char* bitStr = (bit == 2) ? "MARKER" : (bit == 1) ? "ONE   " : "ZERO  ";
      Serial.printf("[TX] Sec %02d: %s (%dms off) [actual: %lums]\n",
        currentSecond, bitStr, lowMs, bitDuration);
    }
    
    // Periodic diagnostics
    if (currentSecond == 59) {
      unsigned long uptimeSec = (millis() - startMillis) / 1000;
      Serial.println();
      Serial.println("--- Frame Complete - Diagnostics ---");
      Serial.printf("  Uptime: %luh %lum %lus\n", 
        uptimeSec / 3600, (uptimeSec % 3600) / 60, uptimeSec % 60);
      Serial.printf("  Frames transmitted: %lu\n", frameCount);
      Serial.printf("  Total bits: %lu\n", totalBitsTransmitted);
      Serial.printf("  Bit timing range: %lu - %lu ms\n", minBitTime, maxBitTime);
      Serial.printf("  WiFi RSSI: %d dBm\n", WiFi.RSSI());
      Serial.printf("  Free heap: %lu bytes\n", ESP.getFreeHeap());
      Serial.println("-----------------------------------");
    }
  }
  
  delay(10);
}
