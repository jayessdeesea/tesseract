/*
 * status_leds.h - Status LED Panel Manager
 * 
 * Drives 3 status LEDs directly from ESP32 GPIO pins:
 *   - Green (GPIO 19): NTP sync status
 *   - Blue  (GPIO 23): WiFi connection status
 *   - Red   (GPIO 25): Transmit activity
 * 
 * Wiring: GPIO → LED anode → 220Ω resistor → GND
 * Current: ~5-7mA per LED (within ESP32 GPIO limits)
 * 
 * Note: Blue LEDs have Vf ≈ 3.0V. At 3.3V GPIO with 220Ω, current
 * is only ~1.4mA which may appear dim. Reduce resistor to 100Ω for
 * blue if needed, or substitute a green LED.
 */

#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <Arduino.h>

/*
 * Initialize all status LED GPIO pins as outputs.
 * Call once from setup().
 */
void statusLedsInit();

/*
 * Update all status LEDs based on current system state.
 * Call from main loop on every iteration (handles its own timing).
 * 
 * Parameters:
 *   wifiConnected    - true if WiFi is currently connected
 *   ntpSyncAgeSec    - seconds since last NTP sync (-1 if never synced)
 *   transmitting     - true if currently transmitting a bit
 */
void statusLedsUpdate(bool wifiConnected, long ntpSyncAgeSec, bool transmitting);

/*
 * Flash the TX LED briefly. Call once per second when transmitting a bit.
 * Non-blocking: sets LED high and records timestamp. statusLedsUpdate()
 * turns it off after LED_TX_FLASH_MS.
 */
void statusLedsTxFlash();

/*
 * Turn all status LEDs off (e.g., for error state or shutdown).
 */
void statusLedsAllOff();

/*
 * Run a startup test sequence: light each LED for 300ms.
 * Useful for visual verification that all LEDs are wired correctly.
 */
void statusLedsSelfTest();

#endif // STATUS_LEDS_H
