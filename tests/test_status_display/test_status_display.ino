/*
 * test_status_display.ino - Status LED & 7-Segment Display Test
 * 
 * Standalone test for the status display interface:
 *   - 3 status LEDs (direct GPIO drive)
 *   - 4× Adafruit HT16K33 7-segment displays (I2C)
 * 
 * Test sequence:
 *   1. I2C bus scan (find all connected displays)
 *   2. LED self-test (cycle each LED)
 *   3. Display self-test (test pattern on each display)
 *   4. Running clock display (counts up) + LED status cycling
 * 
 * Wiring:
 *   GPIO 19 → Green LED → 220Ω → GND  (NTP status)
 *   GPIO 23 → Blue LED  → 220Ω → GND  (WiFi status)
 *   GPIO 25 → Red LED   → 220Ω → GND  (TX status)
 *   GPIO 21 (SDA) → All display SDA
 *   GPIO 22 (SCL) → All display SCL
 *   3.3V → All display VCC
 *   GND  → All display GND
 * 
 * Upload: Arduino IDE → Open this .ino → Select ESP32 board → Upload
 * Monitor: 115200 baud
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

// ============================================================
// Configuration (inlined from config.h)
// Source of truth: wwvb_transmitter/config.h
// Last synced: 2026-03-11
// ============================================================

// GPIO pins
const int LED_NTP_PIN  = 19;   // Green LED: NTP sync status
const int LED_WIFI_PIN = 23;   // Blue LED: WiFi connection status
const int LED_TX_PIN   = 25;   // Red LED: Transmit activity
const int I2C_SDA_PIN  = 21;   // I2C data
const int I2C_SCL_PIN  = 22;   // I2C clock

// Display I2C addresses
const uint8_t DISPLAY_ADDR_YEAR   = 0x70;
const uint8_t DISPLAY_ADDR_DATE   = 0x71;
const uint8_t DISPLAY_ADDR_TIME   = 0x72;
const uint8_t DISPLAY_ADDR_STATUS = 0x73;
const uint8_t DISPLAY_BRIGHTNESS  = 4;
const uint8_t DISPLAY_COUNT       = 4;

// ============================================================
// Display instances
// ============================================================
Adafruit_7segment displays[DISPLAY_COUNT];
const uint8_t dispAddrs[DISPLAY_COUNT] = {
  DISPLAY_ADDR_YEAR, DISPLAY_ADDR_DATE, 
  DISPLAY_ADDR_TIME, DISPLAY_ADDR_STATUS
};
const char* dispNames[DISPLAY_COUNT] = {
  "Year", "Date", "Time", "Status"
};
bool dispAvailable[DISPLAY_COUNT] = {false, false, false, false};

// ============================================================
// Test state
// ============================================================
int testPhase = 0;
unsigned long phaseStart = 0;
int simHour = 12, simMin = 0, simSec = 0;
unsigned long lastSecond = 0;

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("================================================");
  Serial.println("  Status Display Test - LEDs & 7-Segment");
  Serial.println("================================================\n");
  
  // ---- Initialize LED pins ----
  pinMode(LED_NTP_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  pinMode(LED_TX_PIN, OUTPUT);
  digitalWrite(LED_NTP_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(LED_TX_PIN, LOW);
  Serial.println("[LED] GPIO pins initialized (19, 23, 25)");
  
  // ---- I2C bus scan ----
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("\n[I2C] Scanning bus...");
  int totalFound = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[I2C] Device found at 0x%02X", addr);
      // Check if it's one of our expected addresses
      for (int i = 0; i < DISPLAY_COUNT; i++) {
        if (addr == dispAddrs[i]) {
          Serial.printf(" ← %s display", dispNames[i]);
        }
      }
      Serial.println();
      totalFound++;
    }
  }
  Serial.printf("[I2C] Scan complete: %d device(s) found\n\n", totalFound);
  
  // ---- Initialize displays ----
  int dispFound = 0;
  for (int i = 0; i < DISPLAY_COUNT; i++) {
    if (displays[i].begin(dispAddrs[i])) {
      dispAvailable[i] = true;
      dispFound++;
      displays[i].setBrightness(DISPLAY_BRIGHTNESS);
      displays[i].clear();
      displays[i].writeDisplay();
      Serial.printf("[DISP] %s (0x%02X): OK\n", dispNames[i], dispAddrs[i]);
    } else {
      Serial.printf("[DISP] %s (0x%02X): NOT FOUND\n", dispNames[i], dispAddrs[i]);
    }
  }
  Serial.printf("[DISP] %d of %d displays initialized\n\n", dispFound, DISPLAY_COUNT);
  
  // ---- Start test sequence ----
  Serial.println("Starting test sequence...\n");
  phaseStart = millis();
  testPhase = 0;
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
  unsigned long now = millis();
  unsigned long elapsed = now - phaseStart;
  
  switch (testPhase) {
    case 0: testLedSequence(elapsed); break;
    case 1: testDisplayPatterns(elapsed); break;
    case 2: testRunningClock(now); break;
  }
}

// ============================================================
// Phase 0: LED Self-Test (3 seconds total)
// ============================================================
void testLedSequence(unsigned long elapsed) {
  if (elapsed < 500) {
    // All off
    Serial.println("[TEST] Phase 0: LED self-test");
    Serial.println("[TEST] All LEDs off");
    allLedsOff();
  } else if (elapsed < 1000) {
    // Green on
    static bool printed1 = false;
    if (!printed1) { Serial.println("[TEST] NTP LED (green) ON"); printed1 = true; }
    digitalWrite(LED_NTP_PIN, HIGH);
  } else if (elapsed < 1500) {
    // Green off, Blue on
    static bool printed2 = false;
    if (!printed2) { Serial.println("[TEST] WiFi LED (blue) ON"); printed2 = true; }
    digitalWrite(LED_NTP_PIN, LOW);
    digitalWrite(LED_WIFI_PIN, HIGH);
  } else if (elapsed < 2000) {
    // Blue off, Red on
    static bool printed3 = false;
    if (!printed3) { Serial.println("[TEST] TX LED (red) ON"); printed3 = true; }
    digitalWrite(LED_WIFI_PIN, LOW);
    digitalWrite(LED_TX_PIN, HIGH);
  } else if (elapsed < 2500) {
    // All on
    static bool printed4 = false;
    if (!printed4) { Serial.println("[TEST] All LEDs ON"); printed4 = true; }
    digitalWrite(LED_NTP_PIN, HIGH);
    digitalWrite(LED_WIFI_PIN, HIGH);
    digitalWrite(LED_TX_PIN, HIGH);
  } else if (elapsed < 3000) {
    // All off
    allLedsOff();
  } else {
    Serial.println("[TEST] LED self-test complete\n");
    testPhase = 1;
    phaseStart = millis();
  }
}

// ============================================================
// Phase 1: Display Test Patterns (4 seconds)
// ============================================================
void testDisplayPatterns(unsigned long elapsed) {
  static int lastPattern = -1;
  int pattern = elapsed / 1000;
  
  if (pattern != lastPattern) {
    lastPattern = pattern;
    
    switch (pattern) {
      case 0:
        // Show "8888" on all displays (all segments lit)
        Serial.println("[TEST] Phase 1: Display test patterns");
        Serial.println("[TEST] Pattern: 8888 (all segments)");
        for (int i = 0; i < DISPLAY_COUNT; i++) {
          if (!dispAvailable[i]) continue;
          displays[i].writeDigitNum(0, 8);
          displays[i].writeDigitNum(1, 8);
          displays[i].writeDigitNum(3, 8);
          displays[i].writeDigitNum(4, 8);
          displays[i].drawColon(true);
          displays[i].writeDisplay();
        }
        break;
        
      case 1:
        // Show "----" (dash segments)
        Serial.println("[TEST] Pattern: ---- (dashes)");
        for (int i = 0; i < DISPLAY_COUNT; i++) {
          if (!dispAvailable[i]) continue;
          displays[i].writeDigitRaw(0, 0x40);
          displays[i].writeDigitRaw(1, 0x40);
          displays[i].writeDigitRaw(3, 0x40);
          displays[i].writeDigitRaw(4, 0x40);
          displays[i].drawColon(false);
          displays[i].writeDisplay();
        }
        break;
        
      case 2:
        // Show display number on each (1, 2, 3, 4)
        Serial.println("[TEST] Pattern: display IDs (1, 2, 3, 4)");
        for (int i = 0; i < DISPLAY_COUNT; i++) {
          if (!dispAvailable[i]) continue;
          displays[i].clear();
          displays[i].writeDigitNum(4, i + 1);
          displays[i].writeDisplay();
        }
        break;
        
      case 3:
        // Show address on each display
        Serial.println("[TEST] Pattern: I2C addresses");
        for (int i = 0; i < DISPLAY_COUNT; i++) {
          if (!dispAvailable[i]) continue;
          displays[i].clear();
          // Show "0x7N" where N is 0-3
          displays[i].writeDigitNum(0, 0);
          displays[i].writeDigitRaw(1, 0x00); // blank
          displays[i].writeDigitNum(3, 7);
          displays[i].writeDigitNum(4, i);
          displays[i].writeDisplay();
        }
        break;
    }
  }
  
  if (elapsed >= 4000) {
    Serial.println("[TEST] Display test complete\n");
    Serial.println("[TEST] Phase 2: Running clock simulation");
    Serial.println("[TEST] LEDs will cycle through status patterns");
    Serial.println("[TEST] (Running indefinitely - reset to restart)\n");
    testPhase = 2;
    phaseStart = millis();
    lastSecond = millis();
  }
}

// ============================================================
// Phase 2: Running Clock + LED Status Cycling (continuous)
// ============================================================
void testRunningClock(unsigned long now) {
  // Advance simulated clock every second
  if (now - lastSecond >= 1000) {
    lastSecond = now;
    simSec++;
    if (simSec >= 60) { simSec = 0; simMin++; }
    if (simMin >= 60) { simMin = 0; simHour++; }
    if (simHour >= 24) { simHour = 0; }
    
    // Update displays
    updateClockDisplays();
    
    // Brief TX flash
    digitalWrite(LED_TX_PIN, HIGH);
    delay(50);
    digitalWrite(LED_TX_PIN, LOW);
    
    // Print every 10 seconds
    if (simSec % 10 == 0) {
      Serial.printf("[CLOCK] %02d:%02d:%02d UTC\n", simHour, simMin, simSec);
    }
  }
  
  // Cycle LED patterns based on time
  unsigned long cycleMs = now % 10000;  // 10-second cycle
  
  if (cycleMs < 3000) {
    // Simulate: WiFi connected, NTP synced
    digitalWrite(LED_WIFI_PIN, HIGH);
    digitalWrite(LED_NTP_PIN, HIGH);
  } else if (cycleMs < 5000) {
    // Simulate: WiFi connected, NTP aging (slow blink)
    digitalWrite(LED_WIFI_PIN, HIGH);
    digitalWrite(LED_NTP_PIN, (now / 1000) % 2);
  } else if (cycleMs < 7000) {
    // Simulate: WiFi connecting (fast blink), NTP off
    digitalWrite(LED_WIFI_PIN, (now / 300) % 2);
    digitalWrite(LED_NTP_PIN, LOW);
  } else {
    // Simulate: WiFi disconnected, NTP off
    digitalWrite(LED_WIFI_PIN, LOW);
    digitalWrite(LED_NTP_PIN, LOW);
  }
}

// ============================================================
// Helper: Update all clock displays
// ============================================================
void updateClockDisplays() {
  int year = 2026;
  int month = 3;
  int day = 11;
  
  // Display 1: Year
  if (dispAvailable[0]) {
    displays[0].writeDigitNum(0, 2);
    displays[0].writeDigitNum(1, 0);
    displays[0].writeDigitNum(3, 2);
    displays[0].writeDigitNum(4, 6);
    displays[0].drawColon(false);
    displays[0].writeDisplay();
  }
  
  // Display 2: Month.Day
  if (dispAvailable[1]) {
    displays[1].writeDigitNum(0, month / 10);
    displays[1].writeDigitNum(1, month % 10, true);  // dot after month
    displays[1].writeDigitNum(3, day / 10);
    displays[1].writeDigitNum(4, day % 10);
    displays[1].drawColon(false);
    displays[1].writeDisplay();
  }
  
  // Display 3: Hour:Min
  if (dispAvailable[2]) {
    displays[2].writeDigitNum(0, simHour / 10);
    displays[2].writeDigitNum(1, simHour % 10);
    displays[2].writeDigitNum(3, simMin / 10);
    displays[2].writeDigitNum(4, simMin % 10);
    displays[2].drawColon(true);
    displays[2].writeDisplay();
  }
  
  // Display 4: Seconds + Status
  if (dispAvailable[3]) {
    displays[3].writeDigitNum(0, simSec / 10);
    displays[3].writeDigitNum(1, simSec % 10, true);  // dot after seconds
    displays[3].writeDigitAscii(3, 'S');               // Standard time
    displays[3].writeDigitRaw(4, 0x00);                // blank
    displays[3].drawColon(false);
    displays[3].writeDisplay();
  }
}

// ============================================================
// Helper: All LEDs off
// ============================================================
void allLedsOff() {
  digitalWrite(LED_NTP_PIN, LOW);
  digitalWrite(LED_WIFI_PIN, LOW);
  digitalWrite(LED_TX_PIN, LOW);
}
