# Car Control System – FRDM-KL25Z

Embedded smart car control system built using the **NXP FRDM-KL25Z (ARM Cortex-M0+)** board.  
The project includes Bluetooth control, automatic headlights using LDR sensing, environmental monitoring (DHT11 temperature & humidity), **dual ultrasonic obstacle detection (front and rear)**, and a modular FSM-based architecture for vehicle control.


## 🎯 Project Overview

**Sistem de control prin Bluetooth** - Mașinuță inteligentă cu următoarele funcționalități:

- **Control Bluetooth** - comandă prin aplicație mobilă (9600 baud, comenzi single-character)
- **Iluminare automată** - faruri controlate de fotorezistor (LDR) cu prag de 3000 ADC
- **Detecție obstacole duală** - senzori ultrasonici HC-SR04 FRONT și REAR
- **Senzori de mediu** - temperatură și umiditate (DHT11) la cerere
- **Control motoare** - mișcare în 4 direcții (înainte, înapoi, rotire stânga/dreapta 90°)
- **Arhitectură FSM** - mașină cu stări finite pentru control predictibil
- **Evitare automată** - oprire în fața/spatele obstacolelor (prag 20cm)

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
│  │ • DHT11      │  │ • DC Motors  │  │   (UART0)    │         │
│  │ • HC-SR04    │  │   (L293D)    │  │   9600 baud  │         │
│  │   FRONT+REAR │  │              │  │              │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
│         │                  ▲                  ▲                │
│         │                  │                  │                │
│         ▼                  │                  │                │
│  ┌──────────────────────────────────────────────────┐         │
│  │           FSM Control Logic (car_fsm.c)          │         │
│  │  States: IDLE, FORWARD, BACKWARD, LEFT, RIGHT    │         │
│  │  • Auto-lighting (LDR → LED)                     │         │
│  │  • Obstacle Avoidance (FRONT/REAR → Stop)        │         │
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
                    │  • Speed (1-9)  │
                    │  • Sensor Data  │
                    └─────────────────┘
```

### Component Interaction Flow
```
1. Sensor Reading Phase (continuous)
   LDR → ADC → Light Level Decision (threshold: 3000)
   DHT11 → GPIO (1-Wire) → Temp/Humidity (on demand via 'I' command)
   HC-SR04 FRONT → GPIO → Distance (when moving FORWARD)
   HC-SR04 REAR → GPIO → Distance (when moving BACKWARD)

2. Decision Phase (FSM)
   IF FORWARD && front obstacle < 20cm → EVENT_OBSTACLE → IDLE
   IF BACKWARD && rear obstacle < 20cm → EVENT_OBSTACLE → IDLE
   IF dark environment (LDR < 3000) → Turn on lights
   IF Bluetooth command → FSM event → State transition

3. Actuation Phase
   Motors → PWM control via L293D (speed + direction)
   LEDs → GPIO output (headlights)

4. Communication Phase
   Sensor data → Bluetooth → Mobile app
   Commands ← Bluetooth ← Mobile app
```

---

## 📡 Bluetooth Commands

| Command | Alt | Description |
|---------|-----|-------------|
| **Navigation** |||
| F | W | Forward |
| B | X | Backward |
| L | A | Turn Left 90° |
| R | D | Turn Right 90° |
| S | SPACE | Stop |
| **Lights** |||
| O | - | Lights ON (forced) |
| P | - | Lights OFF (forced) |
| M | - | Toggle Auto-mode |
| **Telemetry** |||
| T | - | Get Temperature |
| H | - | Get Humidity |
| U | - | Get Distance |
| I | - | Get All Sensor Info |
| **Speed** |||
| 1-9 | - | Set speed (10%-90%) |

### Telemetry Output Example (Command 'I')
```
=== Sensor Info ===
State: FORWARD
FRONT: 35 cm
REAR: 74 cm
Light: 2733 (ADC)
Temp: 23.0 C
Humidity: 52%
==================
```

### Alert Messages
```
>> State: FORWARD
!! OBSTACLE at 15 cm - STOPPED !!
!! REAR OBSTACLE at 8 cm - STOPPED !!
>> Turn complete -> IDLE
```

---

## 🔌 Pin Mapping

See [pini.md](pini.md) for detailed pin configuration.

### Quick Reference
| Module | Pins |
|--------|------|
| Bluetooth (UART0) | PTA1 (RX), PTA2 (TX) |
| Motor Left | IN1=PTB1, IN2=PTB2, EN=PTA4 (PWM) |
| Motor Right | IN1=PTB3, IN2=PTC2, EN=PTA5 (PWM) |
| Ultrasonic FRONT | TRIG=PTC8, ECHO=PTC9 |
| Ultrasonic REAR | TRIG=PTC8 (shared), ECHO=PTA12 |
| LDR | PTB0 (ADC0_SE8) |
| DHT11 | PTD4 |
| LED Headlight | PTC1 |

---

## 🧩 Software Modules

| Module | Description |
|--------|-------------|
| `PROIECT.c` | Main application, initialization, superloop |
| `car_fsm.c/h` | Finite State Machine for vehicle control |
| `bluetooth.c/h` | UART0 interrupt-driven RX with ring buffer |
| `motor.c/h` | L293D driver, PWM control @ 1kHz |
| `ultrasonic.c/h` | Dual HC-SR04 driver (FRONT + REAR) |
| `dht11.c/h` | Temperature/humidity sensor |
| `ldr.c/h` | Light sensor (ADC) |
| `lights.c/h` | LED headlight control |
| `uart.c/h` | Low-level UART driver |

---

## ⏱️ Timers Used

| Timer | Usage | Configuration |
|-------|-------|---------------|
| TPM0 | Motor PWM (CH1, CH2) | 1kHz, prescaler 4 |
| TPM1 | DHT11 timing | 3MHz, prescaler 16 |
| TPM2 | Ultrasonic timing | 1.5MHz, prescaler 32 |
| PIT0 | FSM turn timing (90° turns) | 400ms one-shot |

---

## 🧪 Test Modes

Enable test modes by uncommenting defines in `PROIECT.c`:

```c
//#define TEST_LDR_LED      // Test LDR + LED auto-lighting
//#define TEST_DHT11        // Test temperature/humidity sensor
//#define TEST_MOTORS       // Test motor movement sequence
//#define TEST_ULTRASONIC   // Test dual ultrasonic sensors
```

---

## 📖 Documentation

- [pini.md](pini.md) - Detailed pin mapping
- [doc/documentatie.tex](doc/documentatie.tex) - Full LaTeX documentation (IEEE format)
- [walkthrough.md](walkthrough.md) - Development walkthrough

---

## 🛠️ Build & Flash

1. Open project in **MCUXpresso IDE**
2. Build: `Project → Build All`
3. Flash: `Run → Debug As → MCUXpresso IDE LinkServer`
4. Connect Bluetooth terminal app at **9600 baud**