# Car Control System – FRDM-KL25Z

Embedded smart car control system built using the **NXP FRDM-KL25Z (ARM Cortex-M0+)** board.  
The project includes Bluetooth control, automatic headlights using LDR sensing, environmental monitoring (DHT11 temperature & humidity), ultrasonic obstacle detection, servo-based radar scanning, and a modular test framework for validating each hardware component independently.

---

## 📖 Documentation

- **[README.md](./README.md)** (this file) - Project overview, quick start, testing
- **[ARCHITECTURE.md](./ARCHITECTURE.md)** - System design, diagrams, protocols, state machines
- **[BREADBOARD_GUIDE.md](./BREADBOARD_GUIDE.md)** - Detailed hardware connections, troubleshooting
- **[PIN_REFERENCE.md](./PIN_REFERENCE.md)** - Complete header pinout map (J1, J2, J9, J10)
- **[PIN_VERIFICATION.md](./PIN_VERIFICATION.md)** - ✅ Pin verification report & bug fixes

---

## 🎯 Project Overview

**Sistem de control prin Bluetooth** - Mașinuță inteligentă cu următoarele funcționalități:

- ✅ **Control Bluetooth** - comandă prin aplicație mobilă
- ✅ **Iluminare automată** - faruri controlate de fotorezistor (LDR)
- 🔄 **Radar ultrasonic** - detecție obstacole cu servomotor (HC-SR04)
- 🔄 **Senzori de mediu** - temperatură și umiditate (DHT11)
- 🔄 **Control motoare** - mișcare în 4 direcții + viteză variabilă
- 🔄 **Sistem integrat** - evitare automată a obstacolelor

---

## 🏗️ System Architecture

### Block Diagram
```
┌─────────────────────────────────────────────────────────────────┐
│                    FRDM-KL25Z (ARM Cortex-M0+)                  │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │   Sensors    │  │  Actuators   │  │    Comms     │         │
│  ├──────────────┤  ├──────────────┤  ├──────────────┤         │
│  │ • LDR (ADC)  │  │ • LED Lights │  │ • Bluetooth  │         │
│  │ • DHT11      │  │ • DC Motors  │  │   (UART)     │         │
│  │ • HC-SR04    │  │ • Servo      │  │ • Debug      │         │
│  │   Ultrasonic │  │              │  │   Console    │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
│         │                  ▲                  ▲                │
│         │                  │                  │                │
│         ▼                  │                  │                │
│  ┌──────────────────────────────────────────────────┐         │
│  │         Control Logic & State Machine            │         │
│  │  • Sensor Fusion                                 │         │
│  │  • Auto-lighting (LDR → LED)                     │         │
│  │  • Obstacle Avoidance (Ultrasonic → Motors)      │         │
│  │  • Environment Monitoring (DHT11 → BT)           │         │
│  │  • Bluetooth Command Parser                      │         │
│  └──────────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  Mobile App     │
                    │  (Bluetooth)    │
                    │  • Direction    │
                    │  • Speed        │
                    │  • Sensor Data  │
                    └─────────────────┘
```

### Component Interaction Flow
```
1. Sensor Reading Phase (50ms cycle)
   LDR → ADC → Light Level Decision
   DHT11 → GPIO (1-Wire) → Temp/Humidity
   HC-SR04 → GPIO (Trigger/Echo) → Distance

2. Decision Phase
   IF obstacle detected → Stop/Turn
   IF dark environment → Turn on lights
   IF Bluetooth command → Execute movement

3. Actuation Phase
   Motors → PWM control (speed + direction)
   Servo → PWM sweep (radar scanning)
   LEDs → GPIO output (headlights)

4. Communication Phase
   Sensor data → Bluetooth → Mobile app
   Commands ← Bluetooth ← Mobile app
```

---

## ✅ Current Status (Implemented)

### 1. Automatic Headlight System (LDR + LED)
- ✅ LDR connected to **PTB0 (ADC0_SE8)**
- ✅ LED connected to **PTC1 (GPIO)**
- ✅ Threshold-based auto-lighting logic
- ✅ Serial debugging via UART0
- ✅ Fully testable via modular test framework

**Files:**
- `source/ldr.c` / `ldr.h`
- `source/lights.c` / `lights.h`
- `source/tests/test_ldr_led.c`

---

## 🔄 Roadmap (In Development)

### 2. Environmental Monitoring (DHT11)
- 🔄 DHT11 sensor on **PTD4 (GPIO - 1-Wire protocol)**
- 🔄 Temperature reading (-40°C to 80°C)
- 🔄 Humidity reading (0% to 100%)
- 🔄 Data transmission via Bluetooth

**Implementation Plan:**
- `source/dht11.c` / `dht11.h` - 1-Wire protocol driver
- `source/env_sensor.c` / `env_sensor.h` - environmental data processing
- `source/tests/test_dht11.c` - DHT11 validation test

### 3. Ultrasonic Radar System (HC-SR04)
- 🔄 Trigger pin: **PTA12 (GPIO)**
- 🔄 Echo pin: **PTA13 (GPIO + Timer Input Capture)**
- 🔄 Servo motor: **PTA1 (TPM2_CH0 - PWM)**
- 🔄 Scanning range: 0° to 180°
- 🔄 Distance measurement: 2cm to 400cm

**Files:**
- `source/ultrasonic.c` / `ultrasonic.h`
- `source/servo.c` / `servo.h`
- `source/tests/test_radar.c`

### 4. Motor Control System
- 🔄 Motor driver: L298N or similar
- 🔄 PWM speed control: **TPM1_CH0/CH1**
- 🔄 Direction control: **PTB2, PTB3, PTB10, PTB11 (GPIO)**
- 🔄 Movement modes: Forward, Backward, Left, Right, Stop

**Files:**
- `source/motors.c` / `motors.h`
- `source/movement.c` / `movement.h`

### 5. Bluetooth Communication (HC-05/HC-06)
- 🔄 UART1 interface: **PTE0 (TX), PTE1 (RX)**
- 🔄 Baud rate: 9600 or 115200
- 🔄 Command protocol (JSON-like):
  ```
  CMD:MOVE:FWD:100    // Move forward at speed 100
  CMD:TURN:LEFT:50    // Turn left at speed 50
  CMD:STOP            // Stop all motors
  REQ:SENSORS         // Request sensor data
  ```
- 🔄 Response format:
  ```
  DATA:TEMP:25.5:HUM:60:DIST:45:LIGHT:1200
  ```

**Files:**
- `source/bluetooth.c` / `bluetooth.h`
- `source/protocol.c` / `protocol.h`

### 6. Integrated Control System
- 🔄 Main control loop with sensor fusion
- 🔄 Autonomous obstacle avoidance
- 🔄 Manual override via Bluetooth
- 🔄 State machine for behavior management

**Files:**
- `source/control.c` / `control.h`
- `source/state_machine.c` / `state_machine.h`

--- 

## 🧪 Modular Testing Framework

Individual hardware modules can be tested independently using compile-time flags in `source/PROIECT.c`.

### Available Tests

```c
// Uncomment ONE test at a time:

#define TEST_LDR_LED       // ✅ LDR + LED auto-lighting
// #define TEST_DHT11      // 🔄 Temperature & humidity
// #define TEST_ULTRASONIC // 🔄 Distance measurement
// #define TEST_SERVO      // 🔄 Servo motor sweep
// #define TEST_MOTORS     // 🔄 Motor control
// #define TEST_BLUETOOTH  // 🔄 BT communication

#ifdef TEST_LDR_LED
    run_test_ldr_led();
#elif defined(TEST_DHT11)
    run_test_dht11();
// ... other tests
#else
    run_main_application();
#endif
```

### Running Tests

#### 1. LDR + LED Test (✅ Working)
```c
#define TEST_LDR_LED
```
- Reads light level from LDR on PTB0
- Automatically controls LED on PTC1
- Displays values via UART
- **Expected behavior:**
  - Cover LDR → LED turns ON
  - Illuminate LDR → LED turns OFF

#### 2. DHT11 Test (🔄 Ready to test)
```c
#define TEST_DHT11
```
- Reads temperature and humidity from DHT11 on PTD4
- Displays readings every 2.5 seconds
- **Hardware setup required:**
  - Connect DHT11 VCC to 3.3V
  - Connect DHT11 DATA to PTD4 with 10kΩ pull-up to 3.3V
  - Connect DHT11 GND to GND
- **Expected output:**
  ```
  Temperature: 25.5°C
  Humidity: 60.0%
  ```

#### 3. General Testing Procedure
1. Connect hardware according to pin mapping
2. Enable desired test in `PROIECT.c`
3. **Build**: Project → Build All (Ctrl+B)
4. **Flash**: Run → Debug (F11)
5. **Monitor**: Terminal tab (COM7, 115200 baud)
6. **Resume**: Press F8 to continue execution
7. Observe serial output and hardware behavior

--- 

## 📁 Project Structure

```
PROIECT/
├── board/                    # Pin & clock configuration
│   ├── board.c, board.h
│   ├── pin_mux.c, pin_mux.h
│   ├── clock_config.c, clock_config.h
│   └── peripherals.c, peripherals.h
│
├── drivers/                  # NXP SDK HAL drivers
│   ├── fsl_gpio.c, fsl_gpio.h
│   ├── fsl_adc16.c, fsl_adc16.h
│   ├── fsl_uart.c, fsl_uart.h
│   ├── fsl_tpm.c, fsl_tpm.h
│   └── ... (other SDK drivers)
│
├── CMSIS/                    # ARM CMSIS core
├── CMSIS_driver/             # CMSIS interface drivers
├── startup/                  # Low-level startup code
├── utilities/                # Debug console utilities
│
├── source/
│   ├── PROIECT.c             # ✅ Main entry point + test dispatcher
│   │
│   ├── ldr.c, ldr.h          # ✅ LDR sensor driver (ADC)
│   ├── lights.c, lights.h    # ✅ LED control (GPIO)
│   ├── dht11.c, dht11.h      # 🔄 DHT11 driver (1-Wire protocol)
│   ├── ultrasonic.c, ultrasonic.h   # 🔄 HC-SR04 driver
│   ├── servo.c, servo.h      # 🔄 Servo motor control (PWM)
│   ├── motors.c, motors.h    # 🔄 DC motor driver
│   ├── bluetooth.c, bluetooth.h     # 🔄 Bluetooth UART
│   ├── protocol.c, protocol.h       # 🔄 Command parser
│   ├── control.c, control.h         # 🔄 Main control logic
│   │
│   ├── tests/
│   │   ├── test_ldr_led.c, test_ldr_led.h      # ✅
│   │   ├── test_dht11.c, test_dht11.h          # 🔄
│   │   ├── test_ultrasonic.c, test_ultrasonic.h # 🔄
│   │   └── ... (other test modules)
│   │
│   ├── mtb.c                 # Micro Trace Buffer
│   └── semihost_hardfault.c  # Fault handler
│
├── README.md                 # This file
├── ARCHITECTURE.md           # System architecture & design
└── PROIECT.mex              # MCUXpresso Config Tools project
```

---

## 🚀 Quick Start Guide

### Prerequisites
- **Hardware**: FRDM-KL25Z board
- **Software**: MCUXpresso IDE
- **Drivers**: OpenSDA USB drivers installed

### Getting Started
1. **Clone the repository**
   ```bash
   git clone https://github.com/antonioheasca4/car-control-bluetooth-system.git
   ```

2. **Open in MCUXpresso**
   - File → Import → Existing Projects into Workspace
   - Select the PROIECT folder

3. **Test LDR + LED (Already Working)**
   - Hardware: Connect LDR to PTB0, LED to PTC1 (as per breadboard)
   - Software: Ensure `#define TEST_LDR_LED` is enabled in PROIECT.c
   - Build & Flash
   - Open serial terminal (COM7, 115200 baud)

4. **Add DHT11 Sensor**
   - Hardware connections:
     ```
     DHT11 Pin 1 (VCC)  → FRDM 3.3V
     DHT11 Pin 2 (DATA) → PTD4 + 10kΩ pull-up to 3.3V
     DHT11 Pin 4 (GND)  → FRDM GND
     ```
   - Software: Change to `#define TEST_DHT11` in PROIECT.c
   - Build & Flash
   - Monitor temperature/humidity readings in terminal

---

## 📚 Documentation

- **README.md** (this file) - Project overview, setup, testing
- **[ARCHITECTURE.md](./ARCHITECTURE.md)** - Detailed system design, diagrams, protocols
- **Inline comments** - Code documentation in source files

---

## 🤝 Contributing

This is an academic project for microprocessor course. Future enhancements welcome:
- Additional sensors integration
- Mobile app development
- Advanced autonomous algorithms
- Power optimization

---

## 📄 License

Educational project - FRDM-KL25Z with NXP SDK

---

## 👤 Author

**Antonio Heasca**
- GitHub: [@antonioheasca4](https://github.com/antonioheasca4)
- Project: Car Control Bluetooth System
- University: ANUL 4 - Microprocesoare

---

## ✨ Project Status

| Module | Status | Description |
|--------|--------|-------------|
| LDR Sensor | ✅ Complete | Automatic lighting based on ambient light |
| LED Control | ✅ Complete | Headlight control via GPIO |
| Debug Console | ✅ Complete | UART serial debugging |
| DHT11 Sensor | 🔄 Ready | Temperature & humidity monitoring |
| HC-SR04 | 📋 Planned | Ultrasonic distance measurement |
| Servo Motor | 📋 Planned | Radar scanning mechanism |
| DC Motors | 📋 Planned | Movement control |
| Bluetooth | 📋 Planned | Wireless communication |
| Autonomous Mode | 📋 Planned | Obstacle avoidance |

**Next Steps**: Test DHT11 integration, then implement ultrasonic radar system.

---

**For detailed architecture, component interactions, and state machines, see [ARCHITECTURE.md](./ARCHITECTURE.md)**

--- 

## 📌 Hardware Pin Configuration

### Available Pins on FRDM-KL25Z Headers

**⚠️ IMPORTANT:** My board has a **LIMITED PIN VARIANT** with only J1 (8 pins), J9 (8 pins), and J10 (6 pins).  
**J2 header is NOT available** on my board!

**Available Pins:**
- **J1:** A1, A2, D4, A12, A4, A5, C8, C9
- **J9:** SDA (PTD5), IOREF, RST, 3.3V, 5V, GND, GND, VIN
- **J10:** B0, B1, B2, B3, C2, C1

### Complete Pin Mapping Table

| Component           | KL25Z Pin | Header Label    | Function        | Notes                          |
|---------------------|-----------|-----------------|-----------------|--------------------------------|
| **Sensors**         |           |                 |                 |                                |
| LDR (Photoresistor) | PTB0      | J10-B0          | ADC0_SE8        | ✅ Implemented & Working       |
| DHT11 Data          | PTD4      | J1-D4           | GPIO (1-Wire)   | 🔄 Ready to test               |
| HC-SR04 Trigger     | PTB2      | J10-B2          | GPIO Output     | ⚠️ Alternative pin suggestion  |
| HC-SR04 Echo        | PTB3      | J10-B3          | GPIO Input/PWM  | ⚠️ Alternative pin suggestion  |
| **Actuators**       |           |                 |                 |                                |
| Headlight LED       | PTC1      | J10-C1          | GPIO Output     | ✅ Implemented & Working       |
| Servo Motor         | PTA12     | J1-A12          | TPM1_CH0 (PWM)  | ⚠️ Alternative pin (or motor)  |
| Motor Direction 1   | PTC8      | J1-C8           | GPIO Output     | ⚠️ Available for future        |
| Motor Direction 2   | PTC9      | J1-C9           | GPIO Output     | ⚠️ Available for future        |
| Motor Speed (PWM)   | PTA12     | J1-A12          | TPM1_CH0 (PWM)  | ⚠️ Share with servo OR motor   |
| **Communication**   |           |                 |                 |                                |
| Debug UART TX       | PTA2      | J1-A2           | UART0_TX        | ✅ Working (via OpenSDA)       |
| Debug UART RX       | PTA1      | J1-A1           | UART0_RX        | ✅ Working (via OpenSDA)       |
| Bluetooth (SW)      | PTA4/PTA5 | J1-A4/J1-A5     | Software Serial | ⚠️ UART1 NOT available         |

**❌ NOT AVAILABLE on my board:**  
PTD0, PTD2, PTD3, PTE0, PTE1, PTA13, PTC10, PTC11 (these require J2 header)

### Power Supply Pins (on headers)
- **3.3V**: J9-3.3V (3.3V output from regulator, ~250mA max)
- **5V**: J9-5V (5V from USB, max 500mA total)
- **GND**: J9-GND (multiple GND pins available)
- **VIN**: J9-VIN (External power input, 7-9V DC)

### Power Guidelines
- **FRDM-KL25Z**: Powered via USB (for programming & debug)
- **Motors**: External 6V-12V battery (separate power, common GND)
- **Servomotor**: 5V from external supply (motors need dedicated power!)
- **DHT11**: 3.3V from J9-4
- **HC-SR04**: 5V from external supply (Echo pin needs voltage divider to 3.3V!)

---

## 🔌 DHT11 Integration Guide

### DHT11 Specifications
- **Operating Voltage**: 3.3V - 5V (use 3.3V from FRDM-KL25Z)
- **Temperature Range**: 0°C to 50°C (±2°C accuracy)
- **Humidity Range**: 20% to 90% RH (±5% accuracy)
- **Protocol**: Single-wire (1-Wire like) communication
- **Response Time**: 2 seconds

### Breadboard Connection
```
DHT11 Pinout (looking at front):
┌─────────────┐
│  DHT11 v2   │
│             │
│  1  2  3  4 │
└─────────────┘
 │  │  │  │
 │  │  │  └─── GND
 │  │  └────── Not connected
 │  └───────── DATA (PTD4)
 └──────────── VCC (3.3V)

Connection:
- Pin 1 (VCC)  → FRDM 3.3V
- Pin 2 (DATA) → PTD4 + 10kΩ pull-up resistor to 3.3V
- Pin 3        → Not connected
- Pin 4 (GND)  → FRDM GND
```

### 1-Wire Communication Protocol
```
Timing Diagram (simplified):
1. Start Signal: MCU pulls DATA low for 18ms, then high for 20-40µs
2. DHT Response: DHT pulls low for 80µs, then high for 80µs
3. Data Transmission: 40 bits (5 bytes)
   - Byte 0: Humidity integer
   - Byte 1: Humidity decimal
   - Byte 2: Temperature integer
   - Byte 3: Temperature decimal
   - Byte 4: Checksum (sum of bytes 0-3)
4. Bit Encoding:
   - '0': 50µs low + 26-28µs high
   - '1': 50µs low + 70µs high
```

---

## 🛠️ Implementation Steps

### Phase 1: DHT11 Driver (Week 1)
1. Create `source/dht11.c` and `dht11.h`
2. Implement 1-Wire protocol:
   - `DHT11_Init()` - configure PTD4 as GPIO
   - `DHT11_Start()` - send start signal
   - `DHT11_ReadBit()` - read single bit with timing
   - `DHT11_ReadByte()` - read 8 bits
   - `DHT11_Read()` - read all 5 bytes + verify checksum
3. Create `source/tests/test_dht11.c` for validation
4. Test and calibrate timing

### Phase 2: Ultrasonic + Servo (Week 2)
1. Implement HC-SR04 driver with Timer Input Capture
2. Implement Servo PWM control (1-2ms pulse, 50Hz)
3. Create radar scanning logic
4. Test obstacle detection

### Phase 3: Motor Control (Week 3)
1. Configure TPM for PWM generation
2. Implement direction control logic
3. Test individual motor movements
4. Integrate with movement commands

### Phase 4: Bluetooth Communication (Week 4)
1. Configure UART1 for Bluetooth module
2. Implement command parser
3. Create protocol handlers
4. Test bidirectional communication

### Phase 5: System Integration (Week 5)
1. Combine all modules in main control loop
2. Implement state machine
3. Add autonomous obstacle avoidance
4. Final testing and calibration

---
