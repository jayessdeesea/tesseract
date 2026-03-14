# WWVB Transmitter Project - Objective

## Mission
Build a low-power 60 kHz WWVB time signal transmitter using an ESP32 to synchronize five atomic clocks in a Seattle home that cannot reliably receive the Fort Collins, CO WWVB signal.

## Success Criteria
- All five WWVB clocks sync reliably from their installed locations
- Continuous 24/7 operation without manual intervention
- NTP time accuracy <10ms via local stratum-1 GPS-disciplined servers
- FCC Part 15.209 compliant (≤40 μV/m @ 300m at 60 kHz)

## Key Rules for AI Assistants

### 1. Keep Specs Current
**ALWAYS update the relevant files in `specs/` when making implementation changes.** If you modify code behavior, update the corresponding spec file to reflect the change. Specs are the single source of truth for this project.

### 2. Spec Files Reference
- `specs/architecture.md` - System architecture & component decisions
- `specs/wwvb_protocol.md` - WWVB encoding specification & bit layout
- `specs/hardware_specs.md` - ESP32, ferrite rod, circuit details
- `specs/fcc_compliance.md` - FCC Part 15 requirements & strategy
- `specs/ntp_integration.md` - NTP client design & failover
- `specs/dst_calculation.md` - Automatic DST algorithm & timezone rules
- `specs/testing_strategy.md` - Unit/integration/hardware test plan
- `specs/implementation_phases.md` - Development phases & milestones
- `specs/references.md` - External links & documentation

### 3. Code Structure
- Main sketch: `wwvb_transmitter/wwvb_transmitter.ino`
- Modular components: `.h`/`.cpp` files in `wwvb_transmitter/`
- Test sketches: `tests/` directory (each test is a standalone `.ino`)
- Target: Arduino IDE with ESP32 board support

### 4. Arduino IDE Constraints
- Arduino IDE opens `.ino` files, not directories. It automatically includes all `.h`/`.cpp` files in the same directory as the `.ino`.
- There is **no `-I` include path** equivalent. All dependencies must be in the same directory as the `.ino` file.
- **Test `.ino` files must be fully self-contained.** Inline all necessary project functions directly in the test file rather than using `#include` to reference files in other directories.
- Each inlined section must include a "Source of truth" comment and "Last synced" date.
- The main program (`wwvb_transmitter/wwvb_transmitter.ino`) works natively because all its modules are in the same directory.

### 5. Keeping Tests in Sync
When modifying code in `wwvb_transmitter/`:
1. Update the canonical source files (`.h`/`.cpp`) in `wwvb_transmitter/`
2. Update the inlined copies in any test files that use those functions
3. Update the "Last synced" date in the test file's inlined section header
4. Run the updated tests to verify correctness

| Source Module | Test Files That Inline It |
|---|---|
| `wwvb_encoder.h/.cpp` | `test_wwvb_encoder.ino`, `hardware_validation.ino` |
| `dst_manager.h/.cpp` | `test_dst_calculation.ino` |
| `ntp_manager.h/.cpp` | *(none — test_ntp_client uses ESP32 libs directly)* |
| `status_leds.h/.cpp` | `test_status_display.ino` (GPIO pins + constants inlined) |
| `display_manager.h/.cpp` | `test_status_display.ino` (display config inlined) |
| `config.h` (GPIO/display defs) | `test_status_display.ino` |

### 6. Development Workflow
- **VS Code**: Primary editor for all code development
- **Arduino IDE**: Compilation and upload only — open individual `.ino` files
- Serial Monitor (115200 baud) for test output and debugging
- Oscilloscope for hardware signal verification

### 7. Communication Style
- Skip introductory explanations
- Focus on implementation details and tradeoffs
- Technical competence assumed (EE degree, ham license, principal SWE)
- Direct, engineering-focused communication
