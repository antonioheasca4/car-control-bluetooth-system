# Pini Folositi - FRDM-KL25Z Car Control

## Port A
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTA1 | UART0_RX | Bluetooth HC-05 | Alt2, 9600 baud |
| PTA2 | UART0_TX | Bluetooth HC-05 | Alt2, 9600 baud |
| PTA4 | TPM0_CH1 | Motor Left PWM | Alt3, 1kHz |
| PTA5 | TPM0_CH2 | Motor Right PWM | Alt3, 1kHz |
| PTA12 | GPIO Input | Ultrasonic ECHO REAR | Cu divizor tensiune 5V→3.3V |

## Port B
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTB0 | ADC0_SE8 | LDR | Analog input, 10k pull-down |
| PTB1 | GPIO Output | Motor Left IN1 | Directie |
| PTB2 | GPIO Output | Motor Left IN2 | Directie |
| PTB3 | GPIO Output | Motor Right IN1 | Directie |

## Port C
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTC1 | GPIO Output | LED Headlight | Far masina |
| PTC2 | GPIO Output | Motor Right IN2 | Directie |
| PTC8 | GPIO Output | Ultrasonic TRIG (SHARED) | Trigger 10µs, partajat FRONT+REAR |
| PTC9 | GPIO Input | Ultrasonic ECHO FRONT | Cu divizor tensiune 5V→3.3V |

## Port D
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTD4 | GPIO I/O | DHT11 Data | Senzor temperatura/umiditate, pull-up intern |

---

## Sumar pe Module

### Motoare (L293D)
- **Motor Stanga**: IN1=PTB1, IN2=PTB2, EN=PTA4 (PWM)
- **Motor Dreapta**: IN1=PTB3, IN2=PTC2, EN=PTA5 (PWM)
- **PWM Frecventa**: 1 kHz
- **Control directie**:
  - Forward: IN1=1, IN2=0
  - Backward: IN1=0, IN2=1
  - Stop: IN1=0, IN2=0

### Bluetooth HC-05 (UART0)
- **TX**: PTA2
- **RX**: PTA1
- **Baud Rate**: 9600
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **RX Buffer**: 32 bytes ring buffer cu intreruperi

### Ultrasonic HC-SR04 (DUAL)
| Senzor | TRIG | ECHO | Utilizare |
|--------|------|------|-----------|
| FRONT | PTC8 | PTC9 | Detectie obstacole miscare inainte |
| REAR | PTC8 (shared) | PTA12 | Detectie obstacole miscare inapoi |

- **IMPORTANT**: Pinii ECHO genereaza 5V! Necesita divizor tensiune (1kΩ + 2kΩ)
- **Prag obstacol**: 20 cm
- **Timeout**: 30 ms

### Senzori
| Senzor | Pin | Tip | Observatii |
|--------|-----|-----|------------|
| LDR | PTB0 | ADC0_SE8 | 12-bit, prag lumina: 3000 |
| DHT11 | PTD4 | GPIO 1-Wire | Citire la cerere (comanda 'I') |

### LED
- **Headlight**: PTC1 (GPIO Output)
- **Mod auto**: Se aprinde cand LDR < 3000 (intuneric)

---

## Timer-e Folosite

| Timer | Canal | Utilizare | Config |
|-------|-------|-----------|--------|
| TPM0 | CH1 | Motor Left PWM | 1kHz, prescaler 4, 48MHz source |
| TPM0 | CH2 | Motor Right PWM | 1kHz, prescaler 4, 48MHz source |
| TPM1 | - | DHT11 timing | 3MHz (48MHz / 16), free-running |
| TPM2 | - | Ultrasonic timing | 1.5MHz (48MHz / 32), free-running |
| PIT | CH0 | FSM turn timer | 400ms one-shot (rotire 90°) |
| PIT | CH1 | Motor test timer | 400ms one-shot (doar in test mode) |

---

## Schema Conexiuni Senzori Ultrasonici

```
                 ┌─────────────┐
    PTC8 ────────┤ TRIG        │
                 │  HC-SR04    │
                 │  (FRONT)    │
    PTC9 ←───────┤ ECHO        │ ← Divizor: 5V → 3.3V
                 └─────────────┘

                 ┌─────────────┐
    PTC8 ────────┤ TRIG        │  (shared cu FRONT)
                 │  HC-SR04    │
                 │  (REAR)     │
    PTA12 ←──────┤ ECHO        │ ← Divizor: 5V → 3.3V
                 └─────────────┘
```

**Divizor tensiune ECHO (5V → 3.3V):**
```
Echo Out ──┬── 1kΩ ──┬── GPIO Pin
           │         │
          2kΩ       GND
           │
          GND
```

---

## Comenzi Bluetooth

| Comanda | Alternativa | Descriere |
|---------|-------------|-----------|
| F | W | Inainte (Forward) |
| B | X | Inapoi (Backward) |
| L | A | Rotire stanga 90° |
| R | D | Rotire dreapta 90° |
| S | SPACE | Stop |
| O | - | Faruri ON |
| P | - | Faruri OFF |
| M | - | Toggle mod auto faruri |
| I | - | Info senzori |
| 1-9 | - | Viteza 10%-90% |