/*
 * wwvb_transmitter.ino - WWVB 60 kHz Time Signal Transmitter
 * 
 * Main Arduino sketch for ESP32-based WWVB transmitter.
 * 
 * Architecture:
 *   1. NTP sync provides UTC time
 *   2. WWVB encoder generates 60-bit frame for next minute
 *   3. Main loop synchronizes to UTC second boundaries
 *   4. Each second: carrier off for bit duration, then carrier on
 *   5. LEDC hardware PWM generates 60 kHz carrier
 * 
 * Hardware:
 *   ESP32 GPIO 18 → 1kΩ → 2N2222 base
 *   2N2222 collector → ferrite coil → 5V (through current limit resistor)
 *   2N2222 emitter → GND
 * 
 * See specs/ directory for detailed documentation.
 */

#include "config.h"
#include "wwvb_encoder.h"
#include "ntp_manager.h"
#include "dst_manager.h"
#include "debug_utils.h"

// ============================================================
// State Variables
// ============================================================
uint8_t wwvbFrame[WWVB_FRAME_SIZE];   // Current minute's time code
int lastSecond = -1;                   // Last processed second
bool frameReady = false;               // Frame has been encoded
bool transmitting = false;             // Currently transmitting
unsigned long frameCount = 0;          // Total frames transmitted
unsigned long uptimeStart = 0;         // millis() at startup


// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);  // Wait for serial monitor
  
  printBanner();
  
  // Configure LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Configure LEDC PWM for 60 kHz carrier
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(WWVB_OUTPUT_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, PWM_DUTY_OFF);  // Start with carrier off
  
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
// Main Loop
// ============================================================
void loop() {
  struct tm timeinfo;
  
  // Get current UTC time
  if (!ntpGetTime(&timeinfo)) {
    // No valid time - blink LED fast and retry
    digitalWrite(STATUS_LED_PIN, (millis() / LED_BLINK_NO_SYNC) % 2);
    ntpMaintain();
    delay(100);
    return;
  }
  
  int currentSecond = timeinfo.tm_sec;
  
  // ---- Detect new second boundary ----
  if (currentSecond != lastSecond) {
    lastSecond = currentSecond;
    
    // ---- At second 0: encode frame for next minute ----
    if (currentSecond == 0) {
      encodeNewFrame(&timeinfo);
    }
    
    // ---- Transmit current second's bit ----
    if (frameReady) {
      transmitBit(wwvbFrame[currentSecond], currentSecond);
    }
    
    // Toggle LED on each second (heartbeat)
    digitalWrite(STATUS_LED_PIN, currentSecond % 2);
    
    // Print status every 10 seconds
    if (currentSecond % 10 == 0) {
      printStatus(&timeinfo, currentSecond, 
                  frameReady ? wwvbFrame[currentSecond] : 0xFF,
                  ntpGetSecondsSinceSync(), ntpGetRSSI());
    }
  }
  
  // ---- Periodic maintenance (non-blocking) ----
  static unsigned long lastMaintenance = 0;
  if (millis() - lastMaintenance > 60000) {  // Every 60 seconds
    lastMaintenance = millis();
    ntpMaintain();
    
    // Print uptime
    unsigned long uptimeSec = (millis() - uptimeStart) / 1000;
    Serial.printf("[SYS] Uptime: %luh %lum | Frames: %lu\n",
      uptimeSec / 3600, (uptimeSec % 3600) / 60, frameCount);
  }
  
  // Small delay to prevent tight-looping but still catch second changes
  delay(10);
}

// ============================================================
// Frame Encoding
// ============================================================
void encodeNewFrame(const struct tm* currentTime) {
  // Copy current time and advance by 1 minute
  // WWVB convention: transmit time for the NEXT minute
  struct tm nextMinute = *currentTime;
  advanceOneMinute(&nextMinute);
  
  // Calculate DST status automatically from UTC time and timezone
  uint8_t dstStatus = calculateDSTStatus(currentTime, DEFAULT_TIMEZONE);
  
  // Encode the frame
  encodeWWVBFrame(&nextMinute, wwvbFrame, dstStatus);
  frameReady = true;
  frameCount++;
  
  // Print frame details
  int doy = calculateDayOfYear(&nextMinute);
  int year = nextMinute.tm_year + 1900;
  const char* dstStr = (dstStatus == DST_IN_EFFECT) ? "DST" :
                       (dstStatus == DST_BEGINS) ? "DST-BEGIN" :
                       (dstStatus == DST_ENDS) ? "DST-END" : "STD";
  Serial.printf("[DST] Auto-calculated: %s (timezone UTC%+d)\n", dstStr, DEFAULT_TIMEZONE);
  printFrameSummary(wwvbFrame, nextMinute.tm_min, nextMinute.tm_hour, doy, year);
}

// ============================================================
// Bit Transmission
// ============================================================
/*
 * Transmit a single WWVB bit by controlling the 60 kHz carrier.
 * 
 * At the start of each second:
 *   1. Turn carrier OFF (reduced power period)
 *   2. Wait for bit-specific duration (200/500/800 ms)
 *   3. Turn carrier ON (full power period)
 *   4. Carrier stays on until next second boundary
 * 
 * This function blocks for the low-power duration.
 * The carrier remains on until the next call.
 */
void transmitBit(uint8_t bitType, int second) {
  int lowMs = getLowDurationMs(bitType);
  
  // Carrier OFF (start of bit)
  ledcWrite(PWM_CHANNEL, PWM_DUTY_OFF);
  
  // Wait for low-power duration
  delay(lowMs);
  
  // Carrier ON (full power for remainder of second)
  ledcWrite(PWM_CHANNEL, PWM_DUTY_ON);
  
  // Debug output (only for markers and bit 1s to reduce serial traffic)
  if (bitType != WWVB_BIT_ZERO) {
    printBitDetail(second, bitType);
  }
}
