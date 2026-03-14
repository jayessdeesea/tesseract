/*
 * wwvb_transmitter.ino - WWVB 60 kHz Time Signal Transmitter
 * 
 * Main Arduino sketch for ESP32-based WWVB transmitter.
 * 
 * Architecture:
 *   1. NTP sync provides UTC time
 *   2. WWVB encoder generates 60-bit frame for current minute
 *   3. Main loop runs on 100ms quantum grid aligned to UTC seconds
 *   4. Each quantum: execute previous action, then calculate next
 *   5. LEDC hardware PWM generates 60 kHz carrier (non-blocking)
 * 
 * Hardware:
 *   ESP32 GPIO 18 → ULN2003AN pin 1 (Input 1)
 *   ULN2003AN pin 16 (Output 1) → ferrite coil → 5V (through current limit resistor)
 *   ULN2003AN pin 8 (GND) → GND
 *   ULN2003AN pin 9 (COM) → 5V (flyback diode return)
 * 
 * See specs/ directory for detailed documentation.
 */

#include "config.h"
#include "wwvb_encoder.h"
#include "ntp_manager.h"
#include "dst_manager.h"
#include "status_leds.h"
#include "display_manager.h"
#include "debug_utils.h"

// ============================================================
// Quantum State Machine
// ============================================================
//
// The main loop runs on a 100ms quantum grid aligned to UTC second
// boundaries. Each quantum follows the execute-first pattern:
//   1. Sleep until next quantum boundary (remainder of 100ms)
//   2. Execute the action calculated in the previous quantum
//   3. Read current state (time, NTP status, etc.)
//   4. Calculate the action for the next quantum
//
// WWVB bit timing maps perfectly to 100ms quanta:
//   Quantum 0: Carrier OFF (start of bit, aligned to second boundary)
//   Quantum 2: Carrier ON if bit=0 (200ms low complete)
//   Quantum 5: Carrier ON if bit=1 (500ms low complete)
//   Quantum 8: Carrier ON if marker (800ms low complete)
//
// Display updates happen at quantum 3 (300ms into second),
// well after carrier transition, in a non-critical timing slot.

const int QUANTUM_MS = 100;            // 100ms quantum period

// Scheduled action types
enum Action : uint8_t {
  ACTION_NONE,
  ACTION_CARRIER_OFF,                  // Turn 60 kHz carrier off
  ACTION_CARRIER_ON,                   // Turn 60 kHz carrier on
};

// State variables
uint8_t wwvbFrame[WWVB_FRAME_SIZE];    // Current minute's time code
int lastSecond = -1;                    // Last processed second
bool frameReady = false;                // Frame has been encoded
bool transmitting = false;              // Currently transmitting
unsigned long frameCount = 0;           // Total frames transmitted
unsigned long uptimeStart = 0;          // millis() at startup
uint8_t currentDstStatus = DST_STANDARD;

// Quantum timing
unsigned long nextQuantumMs = 0;        // millis() target for next quantum
int quantumInSecond = 0;                // 0-9: which quantum within current second
int carrierOnQuantum = -1;              // Which quantum to turn carrier back on
Action pendingAction = ACTION_NONE;     // Action to execute next quantum
bool quantumGridAligned = false;        // True once aligned to UTC


// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);  // Wait for serial monitor
  
  printBanner();
  
  // Configure built-in LED (basic heartbeat)
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Initialize status LED panel and run self-test
  statusLedsInit();
  statusLedsSelfTest();
  
  // Initialize 7-segment displays
  int displaysFound = displayInit();
  if (displaysFound < DISPLAY_COUNT) {
    Serial.printf("[INIT] WARNING: Only %d of %d displays found\n", 
                  displaysFound, DISPLAY_COUNT);
  }
  displayShowConnecting();
  displaySetTimezone(DEFAULT_TIMEZONE);  // Show local time on displays
  
  // Configure LEDC PWM for 60 kHz carrier (ESP32 Core 3.x API)
  ledcAttach(WWVB_OUTPUT_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(WWVB_OUTPUT_PIN, PWM_DUTY_OFF);  // Start with carrier off
  
  Serial.println("[INIT] PWM configured: 60 kHz on GPIO " + String(WWVB_OUTPUT_PIN));
  
  // Initialize NTP
  bool synced = ntpInit(WIFI_SSID, WIFI_PASSWORD,
                        NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  
  if (!synced) {
    Serial.println("[INIT] WARNING: NTP sync failed! Will retry...");
    // Continue anyway - will keep trying in main loop
  }
  
  uptimeStart = millis();
  Serial.println("[INIT] Setup complete. Beginning transmission.\n");
}

// ============================================================
// Main Loop — Quantum State Machine (100ms grid)
// ============================================================
void loop() {
  struct tm timeinfo;
  
  // ---- PHASE 0: SLEEP for remainder of quantum ----
  unsigned long now = millis();
  if (nextQuantumMs > 0 && now < nextQuantumMs) {
    delay(nextQuantumMs - now);
  }
  unsigned long quantumStart = millis();
  
  // ---- PHASE 1: EXECUTE action from previous quantum ----
  switch (pendingAction) {
    case ACTION_CARRIER_OFF:
      ledcWrite(WWVB_OUTPUT_PIN, PWM_DUTY_OFF);
      transmitting = true;
      break;
    case ACTION_CARRIER_ON:
      ledcWrite(WWVB_OUTPUT_PIN, PWM_DUTY_ON);
      break;
    case ACTION_NONE:
    default:
      break;
  }
  pendingAction = ACTION_NONE;
  
  // ---- PHASE 2: READ current state ----
  NtpStatus ntpStatus = ntpGetStatus();
  bool wifiConnected = (ntpStatus != NTP_WIFI_DISCONNECTED);
  long syncAge = ntpGetSecondsSinceSync();
  bool hasError = (ntpStatus == NTP_NOT_SYNCED && syncAge < 0);
  
  if (!ntpGetTime(&timeinfo)) {
    // No valid time — blink LED, retry, don't schedule anything
    digitalWrite(STATUS_LED_PIN, (millis() / LED_BLINK_NO_SYNC) % 2);
    statusLedsUpdate(wifiConnected, syncAge, false);
    ntpMaintain();
    nextQuantumMs = quantumStart + QUANTUM_MS;
    return;
  }
  
  int currentSecond = timeinfo.tm_sec;
  
  // ---- PHASE 3: CALCULATE next actions ----
  
  // Detect new second boundary
  if (currentSecond != lastSecond) {
    lastSecond = currentSecond;
    quantumInSecond = 0;
    
    // Align quantum grid to UTC second boundary
    if (!quantumGridAligned) {
      Serial.println("[SYNC] Quantum grid aligned to UTC second boundary");
      quantumGridAligned = true;
    }
    
    // Calculate DST status every second (cheap math, ensures display
    // shows correct DST status immediately after NTP sync, not just
    // at the next minute boundary when encodeNewFrame() runs)
    currentDstStatus = calculateDSTStatus(&timeinfo, DEFAULT_TIMEZONE);
    
    // Encode new frame at second 0
    if (currentSecond == 0) {
      encodeNewFrame(&timeinfo);
    }
    
    // Schedule carrier OFF for this quantum (we're at the second boundary)
    if (frameReady) {
      // Carrier OFF happens NOW (already in execute phase above if scheduled,
      // but for first second we do it directly)
      ledcWrite(WWVB_OUTPUT_PIN, PWM_DUTY_OFF);
      transmitting = true;
      
      // Calculate which quantum to turn carrier back on
      int lowMs = getLowDurationMs(wwvbFrame[currentSecond]);
      carrierOnQuantum = lowMs / QUANTUM_MS;  // 2, 5, or 8
      
      statusLedsTxFlash();
      
      // Debug output for non-zero bits
      if (wwvbFrame[currentSecond] != WWVB_BIT_ZERO) {
        printBitDetail(currentSecond, wwvbFrame[currentSecond]);
      }
    }
    
    // Toggle built-in LED heartbeat
    digitalWrite(STATUS_LED_PIN, currentSecond % 2);
    
    // Print status every 10 seconds
    if (currentSecond % 10 == 0) {
      printStatus(&timeinfo, currentSecond,
                  frameReady ? wwvbFrame[currentSecond] : 0xFF,
                  syncAge, ntpGetRSSI());
    }
  } else {
    quantumInSecond++;
  }
  
  // Schedule carrier ON at the correct quantum
  if (frameReady && quantumInSecond == carrierOnQuantum - 1) {
    pendingAction = ACTION_CARRIER_ON;  // Will execute next quantum
  }
  
  // Schedule carrier OFF for next second (at quantum 9)
  if (frameReady && quantumInSecond == 9) {
    pendingAction = ACTION_CARRIER_OFF;  // Will execute at second boundary
  }
  
  // Update display at quantum 3 (300ms into second, non-critical slot)
  if (quantumInSecond == 3) {
    bool dstActive = (currentDstStatus == DST_IN_EFFECT ||
                      currentDstStatus == DST_BEGINS);
    displayUpdate(&timeinfo, dstActive, hasError);
  }
  
  // Update status LEDs every quantum (handles blink timing)
  statusLedsUpdate(wifiConnected, syncAge, transmitting);
  
  // Periodic maintenance at quantum 7 of second 30 (once per minute)
  if (quantumInSecond == 7 && lastSecond == 30) {
    ntpMaintain();
    displayRescan();
    unsigned long uptimeSec = (millis() - uptimeStart) / 1000;
    Serial.printf("[SYS] Uptime: %luh %lum | Frames: %lu\n",
      uptimeSec / 3600, (uptimeSec % 3600) / 60, frameCount);
  }
  
  // ---- Schedule next quantum ----
  unsigned long elapsed = millis() - quantumStart;
  nextQuantumMs = quantumStart + QUANTUM_MS;
  if (elapsed > QUANTUM_MS) {
    // Overran quantum — catch up immediately
    nextQuantumMs = millis();
    Serial.printf("[WARN] Quantum overrun: %lums\n", elapsed);
  }
}

// ============================================================
// Frame Encoding
// ============================================================
void encodeNewFrame(const struct tm* currentTime) {
  // Encode the current minute's time
  // WWVB convention: frame identifies the time at the END of the current minute
  // The clock advances its display when the frame completes at second 59
  struct tm frameTime = *currentTime;
  
  // Calculate DST status automatically from UTC time and timezone
  uint8_t dstStatus = calculateDSTStatus(currentTime, DEFAULT_TIMEZONE);
  currentDstStatus = dstStatus;  // Track for display
  
  // Encode the frame
  encodeWWVBFrame(&frameTime, wwvbFrame, dstStatus);
  frameReady = true;
  frameCount++;
  
  // Print frame details
  int doy = calculateDayOfYear(&frameTime);
  int year = frameTime.tm_year + 1900;
  const char* dstStr = (dstStatus == DST_IN_EFFECT) ? "DST" :
                       (dstStatus == DST_BEGINS) ? "DST-BEGIN" :
                       (dstStatus == DST_ENDS) ? "DST-END" : "STD";
  Serial.printf("[DST] Auto-calculated: %s (timezone UTC%+d)\n", dstStr, DEFAULT_TIMEZONE);
  printFrameSummary(wwvbFrame, frameTime.tm_min, frameTime.tm_hour, doy, year);
}

