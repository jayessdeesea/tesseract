/*
 * test_pwm_output.ino - PWM Output and Bit Timing Tests
 * 
 * Tests the 60 kHz carrier generation and WWVB bit timing.
 * REQUIRES: Oscilloscope on GPIO 18 for verification.
 * 
 * Upload to ESP32, open Serial Monitor at 115200 baud.
 * Connect oscilloscope probe to GPIO 18.
 * 
 * Uses ESP32 Arduino Core 3.x LEDC API.
 * 
 * Test sequence:
 *   1. Continuous 60 kHz carrier (verify frequency and duty cycle)
 *   2. Carrier on/off toggle (verify clean transitions)
 *   3. Binary 0 bit (200ms off, 800ms on)
 *   4. Binary 1 bit (500ms off, 500ms on)
 *   5. Marker bit (800ms off, 200ms on)
 *   6. Full 60-second frame transmission
 */

// ============================================================
// Configuration
// ============================================================
const int OUTPUT_PIN     = 18;     // PWM output pin
const int LED_PIN        = 2;      // Status LED
const int PWM_FREQ       = 60000;  // 60 kHz
const int PWM_RESOLUTION = 8;
const int PWM_DUTY_ON    = 128;    // 50% duty cycle
const int PWM_DUTY_OFF   = 0;

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  PWM Output Test Suite");
  Serial.println("  Connect oscilloscope to GPIO 18");
  Serial.println("========================================");
  Serial.println();
  
  // Configure PWM (ESP32 Core 3.x API)
  ledcAttach(OUTPUT_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("PWM configured: 60 kHz, 8-bit resolution, GPIO 18");
  Serial.println("Press Enter in Serial Monitor to advance through tests.");
  Serial.println();
  
  // Run tests sequentially
  testContinuousCarrier();
  testCarrierOnOff();
  testBit0Timing();
  testBit1Timing();
  testMarkerTiming();
  testFullFrame();
  
  Serial.println("========================================");
  Serial.println("  All PWM tests complete!");
  Serial.println("========================================");
}

void loop() {
  // Idle after tests
  delay(10000);
}

// Wait for user to press Enter
void waitForUser(const char* prompt) {
  Serial.println(prompt);
  Serial.println("  >> Press Enter to continue <<");
  while (!Serial.available()) {
    delay(100);
  }
  while (Serial.available()) Serial.read();  // Flush
  Serial.println();
}

// ============================================================
// Test 1: Continuous 60 kHz Carrier
// ============================================================
void testContinuousCarrier() {
  Serial.println("=== Test 1: Continuous 60 kHz Carrier ===");
  Serial.println("Expected on oscilloscope:");
  Serial.println("  - Frequency: 60,000 Hz (±100 Hz)");
  Serial.println("  - Duty cycle: ~50%");
  Serial.println("  - Amplitude: 3.3V peak (ESP32 logic level)");
  Serial.println();
  
  // Turn on carrier
  ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("  Carrier ON - verify with oscilloscope");
  
  waitForUser("  Verify 60 kHz carrier is stable");
  
  // Turn off carrier
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  digitalWrite(LED_PIN, LOW);
}

// ============================================================
// Test 2: Carrier On/Off Toggle
// ============================================================
void testCarrierOnOff() {
  Serial.println("=== Test 2: Carrier On/Off Toggle ===");
  Serial.println("Expected on oscilloscope:");
  Serial.println("  - Clean transitions between on and off");
  Serial.println("  - No ringing or glitches at transitions");
  Serial.println("  - 1 second on, 1 second off, repeating");
  Serial.println();
  
  Serial.println("  Toggling carrier every 1 second (10 cycles)...");
  
  for (int i = 0; i < 10; i++) {
    Serial.printf("  Cycle %d: ON\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    
    Serial.printf("  Cycle %d: OFF\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
  }
  
  waitForUser("  Verify clean on/off transitions");
}

// ============================================================
// Test 3: Binary 0 Timing (200ms off, 800ms on)
// ============================================================
void testBit0Timing() {
  Serial.println("=== Test 3: Binary 0 Bit Timing ===");
  Serial.println("Expected on oscilloscope:");
  Serial.println("  - 200ms carrier OFF");
  Serial.println("  - 800ms carrier ON");
  Serial.println("  - Total: 1000ms per bit");
  Serial.println("  - Repeating for 10 seconds");
  Serial.println();
  
  Serial.println("  Transmitting 10 binary-0 bits...");
  
  for (int i = 0; i < 10; i++) {
    Serial.printf("  Bit %d: OFF 200ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
    delay(200);
    
    Serial.printf("  Bit %d: ON 800ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
    delay(800);
  }
  
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  waitForUser("  Verify 200ms off / 800ms on timing");
}

// ============================================================
// Test 4: Binary 1 Timing (500ms off, 500ms on)
// ============================================================
void testBit1Timing() {
  Serial.println("=== Test 4: Binary 1 Bit Timing ===");
  Serial.println("Expected on oscilloscope:");
  Serial.println("  - 500ms carrier OFF");
  Serial.println("  - 500ms carrier ON");
  Serial.println("  - Total: 1000ms per bit");
  Serial.println("  - Repeating for 10 seconds");
  Serial.println();
  
  Serial.println("  Transmitting 10 binary-1 bits...");
  
  for (int i = 0; i < 10; i++) {
    Serial.printf("  Bit %d: OFF 500ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
    delay(500);
    
    Serial.printf("  Bit %d: ON 500ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
    delay(500);
  }
  
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  waitForUser("  Verify 500ms off / 500ms on timing");
}

// ============================================================
// Test 5: Marker Timing (800ms off, 200ms on)
// ============================================================
void testMarkerTiming() {
  Serial.println("=== Test 5: Marker Bit Timing ===");
  Serial.println("Expected on oscilloscope:");
  Serial.println("  - 800ms carrier OFF");
  Serial.println("  - 200ms carrier ON");
  Serial.println("  - Total: 1000ms per bit");
  Serial.println("  - Repeating for 10 seconds");
  Serial.println();
  
  Serial.println("  Transmitting 10 marker bits...");
  
  for (int i = 0; i < 10; i++) {
    Serial.printf("  Bit %d: OFF 800ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
    delay(800);
    
    Serial.printf("  Bit %d: ON 200ms\n", i + 1);
    ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
    delay(200);
  }
  
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  waitForUser("  Verify 800ms off / 200ms on timing");
}

// ============================================================
// Test 6: Full 60-Second Frame
// ============================================================
void testFullFrame() {
  Serial.println("=== Test 6: Full 60-Second Frame ===");
  Serial.println("Transmitting a test frame with mixed bit types.");
  Serial.println("Pattern: M001 0001M | 0001 0001M | repeating");
  Serial.println("This takes 60 seconds.");
  Serial.println();
  
  // Create a simple test frame
  uint8_t frame[60];
  
  // Fill with a recognizable pattern
  for (int i = 0; i < 60; i++) {
    if (i % 10 == 0 || i == 9 || i == 19 || i == 29 || i == 39 || i == 49 || i == 59) {
      frame[i] = 2;  // Marker at standard positions
    } else if (i % 5 == 0) {
      frame[i] = 1;  // Some binary 1s
    } else {
      frame[i] = 0;  // Binary 0s
    }
  }
  
  // Transmit all 60 bits
  Serial.println("  Starting 60-second frame transmission...");
  
  for (int sec = 0; sec < 60; sec++) {
    int lowMs, highMs;
    const char* typeStr;
    
    switch (frame[sec]) {
      case 0: lowMs = 200; highMs = 800; typeStr = "0"; break;
      case 1: lowMs = 500; highMs = 500; typeStr = "1"; break;
      case 2: lowMs = 800; highMs = 200; typeStr = "M"; break;
      default: lowMs = 200; highMs = 800; typeStr = "?"; break;
    }
    
    Serial.printf("  Sec %02d: %s (%dms off, %dms on)\n", sec, typeStr, lowMs, highMs);
    
    // Carrier OFF
    ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
    digitalWrite(LED_PIN, LOW);
    delay(lowMs);
    
    // Carrier ON
    ledcWrite(OUTPUT_PIN, PWM_DUTY_ON);
    digitalWrite(LED_PIN, HIGH);
    delay(highMs);
  }
  
  ledcWrite(OUTPUT_PIN, PWM_DUTY_OFF);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println();
  Serial.println("  Frame transmission complete!");
  Serial.println();
}
