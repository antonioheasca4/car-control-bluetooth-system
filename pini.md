# Pini Folositi - FRDM-KL25Z Car Control

## Port A
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTA1 | UART0_RX | Bluetooth HC-05 | Alt2 |
| PTA2 | UART0_TX | Bluetooth HC-05 | Alt2 |
| PTA4 | TPM0_CH1 | Motor Left PWM | Alt3 |
| PTA5 | TPM0_CH2 | Motor Right PWM | Alt3 |

## Port B
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTB0 | ADC0_SE8 | LDR | Analog input, 10k pull-up |
| PTB1 | GPIO Output | Motor Left IN1 | Directie |
| PTB2 | GPIO Output | Motor Left IN2 | Directie |
| PTB3 | GPIO Output | Motor Right IN1 | Directie |

## Port C
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTC1 | GPIO Output | LED Headlight | Far masina |
| PTC2 | GPIO Output | Motor Right IN2 | Directie |
| PTC8 | GPIO Output | Ultrasonic TRIG | Trigger 10us |
| PTC9 | GPIO Input | Ultrasonic ECHO | Cu divizor tensiune 5V->3.3V |

## Port D
| Pin | Functie | Modul | Observatii |
|-----|---------|-------|------------|
| PTD4 | GPIO I/O | DHT11 Data | Senzor temperatura/umiditate |

---

## Sumar pe Module

### Motoare (L293D)
- **Motor Stanga**: IN1=PTB1, IN2=PTB2, EN=PTA4 (PWM)
- **Motor Dreapta**: IN1=PTB3, IN2=PTC2, EN=PTA5 (PWM)

### Bluetooth HC-05 (UART0)
- TX: PTA2, RX: PTA1

### Ultrasonic HC-SR04
- TRIG: PTC8, ECHO: PTC9 (cu divizor tensiune!)

### Senzori
- LDR: PTB0 (ADC)
- DHT11: PTD4 (GPIO)

### LED
- Headlight: PTC1

---

## Timer-e Folosite
| Timer | Utilizare | Config |
|-------|-----------|--------|
| TPM0 | Motor PWM (CH1, CH2) | 1kHz, prescaler 4 |
| TPM1 | Liber | - |
| TPM2 | Ultrasonic timing | 1.5MHz, prescaler 32 |