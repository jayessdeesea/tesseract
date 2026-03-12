/*
 * test_gpio_voltage.ino - GPIO Voltage Level Test
 * 
 * Tests GPIO 18 and GPIO 19 with a slow 0.5 Hz toggle (1 second HIGH, 1 second LOW)
 * so you can verify voltage levels with a multimeter.
 * 
 * Expected: Each pin should alternate between ~3.3V and ~0V.
 * If a pin reads significantly less than 3.3V when HIGH, it may be damaged.
 * 
 * Hardware:
 *   - Connect multimeter (DC volts) between GPIO pin and GND
 *   - No other connections needed - disconnect everything from GPIO 18/19
 * 
 * Serial Monitor: 115200 baud - prints current state each toggle
 */

const int PIN_A = 18;  // Primary PWM pin
const int PIN_B = 19;  // Comparison pin

// 20 Hz = 50ms period = 25ms HIGH + 25ms LOW
const int HALF_PERIOD_MS = 25;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("  GPIO Voltage Level Test");
  Serial.println("  Pins: GPIO 18 and GPIO 19");
  Serial.println("  Frequency: 20 Hz (50ms period)");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Oscilloscope: 10ms/div, 1V/div, trigger ~1.5V rising edge");
  Serial.println("Expected: 0V to 3.3V square wave on both pins");
  Serial.println();
  
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  
  // Start with both LOW
  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  
  Serial.println("Running...");
}

void loop() {
  digitalWrite(PIN_A, HIGH);
  digitalWrite(PIN_B, HIGH);
  delay(HALF_PERIOD_MS);
  
  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  delay(HALF_PERIOD_MS);
}
