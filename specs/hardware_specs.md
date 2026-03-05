# Hardware Specifications

## ESP32 DevKit
- **Part:** HiLetgo ESP32-WROOM-32D DevKit (Amazon B08D5ZD528)
- **Chip:** ESP32-WROOM-32D/E
- **USB-Serial:** CP2102
- **Regulator:** 3.3V onboard
- **Layout:** 38-pin breadboard compatible
- **Connector:** Micro-USB
- **Power consumption:** 80mA idle + WiFi, ~100-150mA transmitting
- **GPIO:** All 3.3V (not 5V tolerant)
- **Clock:** Dual-core 240 MHz

### Pin Selection
- **PWM Output:** GPIO 18 (safe pin, no strapping function)
- **Status LED:** GPIO 2 (built-in LED on most DevKits)
- **Avoid:** GPIO 0, 2, 5, 12, 15 (strapping pins with boot implications)
- **Safe GPIOs:** 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33

### LEDC PWM Peripheral
- 16 independent PWM channels (8 high-speed, 8 low-speed)
- Hardware-generated waveform (no CPU involvement once configured)
- Frequency range: 1 Hz to 40 MHz
- Resolution: 1-20 bits
- **Our config:** Channel 0, 60 kHz, 8-bit resolution (0-255 duty)

## Ferrite Rod Antenna
- **Dimensions:** 10mm diameter × 200mm length
- **Material:** Type 61 or 77 (high permeability at LF frequencies)
- **Permeability:** μ ≈ 125 (material 61) or μ ≈ 2000 (material 77)

### Coil Winding
- **Wire:** 30 AWG enameled copper (magnet wire)
- **Turns:** 200
- **Coil length:** ~50mm on the rod (tightly wound, single layer)
- **Expected inductance:** ~10-20 mH
- **Wire needed:** ~50 feet

### Winding Instructions
1. Leave 6" of wire free at start
2. Wind 200 turns tightly, side-by-side (no overlap)
3. Mark every 50 turns with tape for counting
4. Leave 6" of wire free at end
5. Secure coil with tape or clear nail polish
6. Scrape enamel off wire ends for electrical contact

### Why 200mm Rod
- Magnetic moment ∝ rod length
- 2× length ≈ 2× field strength vs 100mm rod
- Better whole-house coverage
- Can always reduce power if too strong

## Transistor Driver Circuit

### Schematic
```
         5V (USB)
          |
          R2 (optional current limit, 10-100Ω)
          |
     Ferrite Coil (200 turns)
          |
          Collector
ESP32 GPIO 18 ---[R1 1kΩ]--- Base   2N2222
          |                   Emitter
          |                     |
         GND                  GND
```

### 2N2222 NPN Transistor
- **Package:** TO-92
- **Max Ic:** 800 mA
- **Max Vce:** 40V
- **hFE (gain):** 100-300
- **fT:** 300 MHz (well above 60 kHz)

### Component Values
- **R1 (base resistor):** 1kΩ
  - GPIO voltage: 3.3V
  - Base current: (3.3V - 0.7V) / 1kΩ ≈ 2.6 mA
  - With hFE=100, can switch up to 260 mA collector current
- **R2 (current limit):** Start with 100Ω, reduce if more range needed
  - Limits coil current to ~50 mA at 5V
  - Power dissipation: 0.25W (1/4W resistor OK)

### Resonant Circuit (Optional Enhancement)
- Add capacitor in series with coil to resonate at 60 kHz
- C = 1 / (4π²f²L)
- For L=15mH: C ≈ 470 pF
- Increases Q factor, improves efficiency
- Only add if range is insufficient without it

## Power Supply
- **Source:** USB wall adapter (5V, 1A minimum)
- **ESP32 power:** Via Micro-USB connector
- **Antenna power:** Via 5V pin on DevKit (USB passthrough)
- **Total consumption:** ~150-200 mA at 5V (~1W)

## Breadboard Layout
```
+5V rail ─────────────────────────────────
                    R2
              ┌─────┤
              │  Ferrite Coil
              │     │
              └─ C ─┘  (2N2222)
                 │ B
         R1      │
GPIO 18 ──┤──── B
                 E
                 │
GND rail ─────────────────────────────────
```
