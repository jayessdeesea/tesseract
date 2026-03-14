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
    analogWrite(LED_WIFI_PIN, LED_BRIGHTNESS);  // Solid = connected
  } else {
    // Fast blink = trying to connect/reconnect
    analogWrite(LED_WIFI_PIN, ((now / LED_BLINK_WIFI_CONN) % 2) ? LED_BRIGHTNESS : 0);
  }
  
  // ---- NTP LED (Green, GPIO 19) ----
  if (ntpSyncAgeSec < 0) {
    // Never synced
    analogWrite(LED_NTP_PIN, 0);
  } else if ((unsigned long)ntpSyncAgeSec < NTP_SYNC_FRESH_SEC) {
    // Fresh sync — solid green
    analogWrite(LED_NTP_PIN, LED_BRIGHTNESS);
  } else if ((unsigned long)ntpSyncAgeSec < NTP_SYNC_STALE_SEC) {
    // Aging — slow blink
    analogWrite(LED_NTP_PIN, ((now / LED_BLINK_NTP_AGING) % 2) ? LED_BRIGHTNESS : 0);
  } else {
    // Stale (>24 hours) — off
    analogWrite(LED_NTP_PIN, 0);
  }
  
  // ---- TX LED (Red, GPIO 25) ----
  if (txFlashActive && (now - txFlashStart >= LED_TX_FLASH_MS)) {
    analogWrite(LED_TX_PIN, 0);
    txFlashActive = false;
  }
}


void statusLedsTxFlash() {
  analogWrite(LED_TX_PIN, LED_BRIGHTNESS);
  txFlashStart = millis();
  txFlashActive = true;
}


void statusLedsAllOff() {
  analogWrite(LED_NTP_PIN, 0);
  analogWrite(LED_WIFI_PIN, 0);
  analogWrite(LED_TX_PIN, 0);
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
