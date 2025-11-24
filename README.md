# Car Control System – FRDM-KL25Z

Embedded smart car control system built using the **NXP FRDM-KL25Z (ARM Cortex-M0+)** board.  
The project includes Bluetooth control, automatic headlights using LDR sensing, environmental monitoring, ultrasonic obstacle detection, servo-based radar scanning, and a modular test framework for validating each hardware component independently.

---

## Current Status (Implemented)

### Automatic Headlight System (LDR + LED)
- LDR connected to **PTB0 (ADC0_SE8)**
- LED connected to **PTC1 (GPIO)**
- Threshold-based auto-lighting logic
- Fully testable via the modular test system

**Files:**
- source/ldr.c
- source/ldr.h
- source/lights.c
- source/lights.h
- source/tests/test_ldr_led.c
- source/tests/test_ldr_led.h

---

## Roadmap
- Coming next:
### Ultrasonic module (HC-SR04)
### Servo motor PWM control (radar/scan)
### Motor driver + direction control
### Bluetooth UART command interface
### Integrated movement logic with obstacle avoidance

--- 

## Modular Test

Individual hardware modules can be tested independently using compile-time flags.

Example (in `source/PROIECT.c`):

```c
#define TEST_LDR_LED

// Main automatically dispatches to the selected test:
#ifdef TEST_LDR_LED
    run_test_ldr_led();
#else
    run_main_application();
#endif
```

--- 

## Running the LDR + LED Test

1. Enable the test in PROIECT.c:
```c
#define TEST_LDR_LED
```
2. Build & Debug using MCUXpresso
3. Open a terminal
4. Cover the LDR -> LED turns ON
5. Illuminate the LDR → LED turns OFF

--- 

## Project Structure
```
project-root/
├── board/              # pin + clock configuration (generated)
├── drivers/            # NXP SDK drivers
├── CMSIS/              # ARM CMSIS core
├── CMSIS_driver/       # CMSIS drivers
├── startup/            # low-level startup code
├── utilities/          # debug console / printf utils
├── source/
│   ├── PROIECT.c       # main + test dispatcher
│   ├── ldr.c
│   ├── ldr.h
│   ├── lights.c
│   ├── lights.h
│   ├── tests/
│   │   ├── test_ldr_led.c
│   │   └── test_ldr_led.h
│   ├── mtb.c
│   └── semihost_hardfault.c
├── PROIECT.mex         # MCUXpresso ConfigTools project file
├── .project
├── .cproject
└── .settings/
```

--- 

## Hardware Pin Configuration
```
| Component | KL25Z Pin | Function   |
|----------|-----------|------------|
| LDR      | PTB0      | ADC0_SE8   |
| LED      | PTC1      | GPIO       |
```
