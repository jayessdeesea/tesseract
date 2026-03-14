/*
 * status_leds.cpp - Status LED Panel Manager Implementation
 * 
 * LED behavior:
 *   NTP (Green):  Solid = synced (<1hr), Slow blink = aging (>1hr), Off = failed/stale
 *   WiFi (Blue):  Solid = connected, Fast blink = connecting, Off = disconnected
 *   TX (Red):     Brief flash each second = transmitting, Off = idle
 */

#include "status_leds.h"
#include "config.h"

// TX flash state
static unsigned long txFlashStart = 0;
static bool txFlashActive = false;


void statusLedsInit() {
  pinMode(LED_NTP_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  pinMode(LED_TX_PIN, OUTPUT);
  
  digitalWrite(LED_NTP_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(LED_TX_PIN, LOW);
  
  Serial.println("[LED] Status LEDs initialized (GPIO 19/23/25)");
}


void statusLedsUpdate(bool wifiConnected, long ntpSyncAgeSec, bool transmitting) {
  unsigned long now = millis();
  
  // ---- WiFi LED (Blue, GPIO 23) ----
  if (wifiConnected) {
    digitalWrite(LED_WIFI_PIN, HIGH);  // Solid = connected
  } else {
    // Fast blink = trying to connect/reconnect
    digitalWrite(LED_WIFI_PIN, (now / LED_BLINK_WIFI_CONN) % 2);
  }
  
  // ---- NTP LED (Green, GPIO 19) ----
  if (ntpSyncAgeSec < 0) {
    // Never synced
    digitalWrite(LED_NTP_PIN, LOW);
  } else if ((unsigned long)ntpSyncAgeSec < NTP_SYNC_FRESH_SEC) {
    // Fresh sync — solid green
    digitalWrite(LED_NTP_PIN, HIGH);
  } else if ((unsigned long)ntpSyncAgeSec < NTP_SYNC_STALE_SEC) {
    // Aging — slow blink
    digitalWrite(LED_NTP_PIN, (now / LED_BLINK_NTP_AGING) % 2);
  } else {
    // Stale (>24 hours) — off
    digitalWrite(LED_NTP_PIN, LOW);
  }
  
  // ---- TX LED (Red, GPIO 25) ----
  if (txFlashActive && (now - txFlashStart >= LED_TX_FLASH_MS)) {
    digitalWrite(LED_TX_PIN, LOW);
    txFlashActive = false;
  }
}


void statusLedsTxFlash() {
  digitalWrite(LED_TX_PIN, HIGH);
  txFlashStart = millis();
  txFlashActive = true;
}


void statusLedsAllOff() {
  digitalWrite(LED_NTP_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(LED_TX_PIN, LOW);
  txFlashActive = false;
}


void statusLedsSelfTest() {
  Serial.println("[LED] Self-test: NTP (green)...");
  digitalWrite(LED_NTP_PIN, HIGH);
  delay(300);
  digitalWrite(LED_NTP_PIN, LOW);
  
  Serial.println("[LED] Self-test: WiFi (blue)...");
  digitalWrite(LED_WIFI_PIN, HIGH);
  delay(300);
  digitalWrite(LED_WIFI_PIN, LOW);
  
  Serial.println("[LED] Self-test: TX (red)...");
  digitalWrite(LED_TX_PIN, HIGH);
  delay(300);
  digitalWrite(LED_TX_PIN, LOW);
  
  Serial.println("[LED] Self-test complete");
}
