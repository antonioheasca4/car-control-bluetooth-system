# Car Control System – FRDM-KL25Z

Embedded smart car control system built using the **NXP FRDM-KL25Z (ARM Cortex-M0+)** board.  
The project includes Bluetooth control, automatic headlights using LDR sensing, environmental monitoring (DHT11 temperature & humidity), ultrasonic obstacle detection, servo-based radar scanning, and a modular test framework for validating each hardware component independently.


## 🎯 Project Overview

**Sistem de control prin Bluetooth** - Mașinuță inteligentă cu următoarele funcționalități:

- **Control Bluetooth** - comandă prin aplicație mobilă
- **Iluminare automată** - faruri controlate de fotorezistor (LDR)
- **Radar ultrasonic** - detecție obstacole cu servomotor (HC-SR04)
- **Senzori de mediu** - temperatură și umiditate (DHT11)
- **Control motoare** - mișcare în 2 direcții
- **Sistem integrat** - evitare automată a obstacolelor

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