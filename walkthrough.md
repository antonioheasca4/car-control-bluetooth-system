# Ghid COMPLET de Cablare și Asamblare - Mașinuță Bluetooth

## Înainte de a începe

### Ce ai nevoie:
- [ ] Placa FRDM-KL25Z
- [ ] Breadboard (min 400 puncte)
- [ ] Mașinuță cu 2 motoare DC și suport baterii 4xAA
- [ ] IC L293D (driver motoare)
- [ ] Modul Bluetooth HC-05 sau HC-06
- [ ] Senzor ultrasonic HC-SR04
- [ ] Senzor DHT11 (3 pini)
- [ ] Fotorezistor (LDR)
- [ ] 2x LED (faruri - orice culoare, preferabil albe)
- [ ] Rezistențe: 2x 330Ω (pentru LED-uri), 1x 1kΩ, 1x 2kΩ (sau 2x 1kΩ în serie), 1x 10kΩ
- [ ] ~20 fire jumper (preferabil: 10 tată-tată, 10 tată-mamă)
- [ ] 4x baterii AA

---

## Convenții în acest ghid

**Breadboard-ul:** Rândurile sunt numerotate 1-30, coloanele sunt a-e (stânga) și f-j (dreapta).

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
 1 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
 2 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
 3 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
   │    ...         ...     │
30 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │
   └─────────────────────────┘
   ═══════════════════════════ (+) Linie alimentare ROȘIE
   ═══════════════════════════ (-) Linie alimentare ALBASTRĂ/NEAGRĂ
```

**Notații:**
- `(rând, coloană)` = poziție pe breadboard, ex: `(5, a)` = rândul 5, coloana a
- `→` = conectează la
- `FRDM.PTC1` = pinul PTC1 de pe placa FRDM-KL25Z

---

## PASUL 1: Pregătirea Breadboard-ului

### 1.1 Pune breadboard-ul pe masă
Orientează-l cu liniile de alimentare (+) și (-) în partea de jos.

### 1.2 Alimentare de la baterii
Mașinuța are un suport pentru 4 baterii AA cu un switch ON/OFF.

**Identifică firele din suportul de baterii:**
- **Firul ROȘU** = Plus (+) = 6V
- **Firul NEGRU** = Minus (-) = GND

**Conexiuni:**
| De la | La | Notă |
|-------|-----|------|
| Fir ROȘU baterii (după switch) | Linia (+) roșie a breadboard-ului | Când switch-ul e ON, ai 6V aici |
| Fir NEGRU baterii | Linia (-) neagră a breadboard-ului | Asta e GND comun |

> [!IMPORTANT]
> Switch-ul de pe mașinuță trebuie să fie pe **OFF** când faci conexiunile!

---

## PASUL 2: Plasează L293D pe Breadboard

L293D are 16 pini. Pune-l **în mijlocul** breadboard-ului, cu crestătura în sus.

### 2.1 Poziționare exactă

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
 5 │ ● ● ● ● ●   ● ● ● ● ● │  ← L293D ocupă rândurile 5-12
 6 │ ● ● ● ● ●   ● ● ● ● ● │
 7 │ ● ● ● ● ●   ● ● ● ● ● │
 8 │ ● ● ● ● ●   ● ● ● ● ● │
 9 │ ● ● ● ● ●   ● ● ● ● ● │
10 │ ● ● ● ● ●   ● ● ● ● ● │
11 │ ● ● ● ● ●   ● ● ● ● ● │
12 │ ● ● ● ● ●   ● ● ● ● ● │
   └─────────────────────────┘
```

**Plasare fizică:**
1. Ține L293D cu **crestătura (în formă de U) în sus**
2. Pinul 1 este în **stânga-sus** (lângă crestătură)
3. Pune pinii din stânga în coloana `e`, pinii din dreapta în coloana `f`
4. Pinul 1 (EN1) va fi la poziția `(5, e)`
5. Pinul 16 (VCC1) va fi la poziția `(5, f)`

**După plasare, pinii sunt astfel:**

| Pin L293D | Funcție | Poziție Breadboard |
|-----------|---------|-------------------|
| Pin 1 (EN1) | Enable Motor Stânga | (5, e) |
| Pin 2 (IN1) | Direcție Motor Stânga | (6, e) |
| Pin 3 (OUT1) | Ieșire Motor Stânga + | (7, e) |
| Pin 4 (GND) | Masă | (8, e) |
| Pin 5 (GND) | Masă | (9, e) |
| Pin 6 (OUT2) | Ieșire Motor Stânga - | (10, e) |
| Pin 7 (IN2) | Direcție Motor Stânga | (11, e) |
| Pin 8 (VCC2) | Alimentare Motoare 6V | (12, e) |
| Pin 9 (EN2) | Enable Motor Dreapta | (12, f) |
| Pin 10 (IN3) | Direcție Motor Dreapta | (11, f) |
| Pin 11 (OUT3) | Ieșire Motor Dreapta + | (10, f) |
| Pin 12 (GND) | Masă | (9, f) |
| Pin 13 (GND) | Masă | (8, f) |
| Pin 14 (OUT4) | Ieșire Motor Dreapta - | (7, f) |
| Pin 15 (IN4) | Direcție Motor Dreapta | (6, f) |
| Pin 16 (VCC1) | Alimentare Logică 3.3V | (5, f) |

---

## PASUL 3: Conectează L293D la Alimentare

### 3.1 Conexiuni GND (4 fire scurte sau jumper)

| De la (poziție) | La |
|-----------------|-----|
| (8, a) = lângă Pin 4 | Linia (-) GND |
| (9, a) = lângă Pin 5 | Linia (-) GND |
| (8, j) = lângă Pin 13 | Linia (-) GND |
| (9, j) = lângă Pin 12 | Linia (-) GND |

### 3.2 Alimentare Motoare (Pin 8 = VCC2)

| De la (poziție) | La |
|-----------------|-----|
| (12, a) = lângă Pin 8 | Linia (+) 6V (de la baterii) |

### 3.3 Alimentare Logică (Pin 16 = VCC1)

Aceasta vine de la FRDM-KL25Z (3.3V). O facem la Pasul 7.

---

## PASUL 4: Conectează Motoarele la L293D

Mașinuța are 2 motoare. Fiecare motor are 2 fire (de obicei roșu și negru, sau 2 de aceeași culoare).

### 4.1 Identifică motoarele

Pune mașinuța în fața ta:
- **Motor STÂNGA** = roata din stânga
- **Motor DREAPTA** = roata din dreapta

### 4.2 Motor STÂNGA

| Fir Motor Stânga | Conectează la | Poziție Breadboard |
|------------------|---------------|-------------------|
| Fir 1 (oricare) | OUT1 (Pin 3) | (7, a) sau (7, b) |
| Fir 2 (celălalt) | OUT2 (Pin 6) | (10, a) sau (10, b) |

### 4.3 Motor DREAPTA

| Fir Motor Dreapta | Conectează la | Poziție Breadboard |
|-------------------|---------------|-------------------|
| Fir 1 (oricare) | OUT3 (Pin 11) | (10, h) sau (10, i) |
| Fir 2 (celălalt) | OUT4 (Pin 14) | (7, h) sau (7, i) |

> [!TIP]
> **Dacă motorul merge în sens opus:** Inversează cele 2 fire ale motorului respectiv.

---

## PASUL 5: Montează Senzorul Ultrasonic HC-SR04

HC-SR04 are 4 pini: VCC, TRIG, ECHO, GND.

> [!IMPORTANT]
> HC-SR04 necesită **5V** pentru a funcționa! La 3.3V transmițătorul ultrasonic nu are putere suficientă.
> ECHO trimite 5V, deci necesită **divizor de tensiune** spre PTC9!

### 5.1 Alimentare 5V pe Rândul 25

Folosim rândul 25 ca linie de 5V pentru mai multe module:

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
25 │ ● ● ○ ○ ●   ○ ○ ○ ○ ○ │  ← 25a: FRDM.P5V_USB, 25b: HC-SR04 VCC, 25e: Bluetooth VCC
   └─────────────────────────┘
```

### 5.2 Poziționare HC-SR04

Pune HC-SR04 pe rândurile **30-33** (doar TRIG și ECHO folosesc pinii, VCC și GND conectați separat):

```
        a     b     c     d     e       f g h i j
      ┌───────────────────────────────────────────┐
  30  │ VCC→  ○     ○     ○     ○       ○ ○ ○ ○ ○ │  ← Pin VCC - conectat la 25b
  31  │ TRIG  ○     ○     ○     ●       ○ ○ ○ ○ ○ │  ← Pin TRIG (31, a), fir PTC8 (31, e)
  32  │ ECHO  ○     ●     ○     ○       ○ ○ ○ ○ ○ │  ← Pin ECHO (32, a), Rezistor 1kΩ (32, c)
  33  │ GND→  ○     ○     ○     ○       ○ ○ ○ ○ ○ │  ← Pin GND - conectat la Linia (-)
      └───────────────────────────────────────────┘
```

### 5.3 Conexiuni HC-SR04

| Pin HC-SR04 | Poziție | Conectează la | Notă |
|-------------|---------|---------------|------|
| **VCC** | jumper de la 30 | **(25, b)** | Linia 5V |
| **TRIG** | (31, a) | **(31, e)** → **FRDM.PTC8** | Direct |
| **ECHO** | (32, a) | Divizor (vezi 5.4) | NU direct! |
| **GND** | jumper de la 33 | **Linia (-)** | Direct la GND |

### 5.4 DIVIZOR DE TENSIUNE pentru ECHO (OBLIGATORIU!)

> [!CAUTION]
> **HC-SR04 ECHO trimite 5V! FRDM.PTC9 suportă doar 3.3V!**
> Fără divizor, poți arde pinul PTC9!

**Schema:**
```
ECHO (32, a) ───[1kΩ]───┬─── FRDM.PTC9
                        │
                     [2kΩ]
                        │
                       GND
```

**Poziții pe breadboard:**

```
        a     b     c     d     e       f g h i j
      ┌───────────────────────────────────────────┐
  32  │ ECHO  ○     ●     ○     ○       ○ ○ ○ ○ ○ │  ← ECHO (32,a), Rezistor 1kΩ (32,c)
  33  │ ○     ○     ○     ○     ○       ○ ○ ○ ○ ○ │
  34  │ ○     ○     ●     ●     ●       ○ ○ ○ ○ ○ │  ← 1kΩ (34,c) + 2kΩ (34,d) + PTC9 (34,e)
  35  │ ○     ○     ○     ●     ○       ○ ○ ○ ○ ○ │  ← 2kΩ (35,d) → GND
      └───────────────────────────────────────────┘
```

**Pas cu pas:**

1. **Rezistor 1kΩ**: Un picior în **(32, c)**, celălalt în **(34, c)**
2. **Rezistor 2kΩ**: Un picior în **(34, d)**, celălalt în **(35, d)**
3. **Fir GND**: De la **(35, d)** sau alt punct pe rând 35 la linia (-)
4. **Fir PTC9**: De la **(34, e)** la **FRDM.PTC9** (J1)

> [!TIP]
> (34, c), (34, d) și (34, e) sunt pe același rând, deci sunt **conectate electric**!
> Formula: V_out = 5V × (2kΩ / (1kΩ + 2kΩ)) = **3.3V** ✅

---

## PASUL 6: Montează Senzorul DHT11

DHT11 (versiunea cu 3 pini) are: VCC, DATA, GND.

### 6.1 Poziționare

> [!IMPORTANT]
> Pinii se pun în **coloana f**, iar firele se conectează pe **stânga (coloane a-e)** 
> pentru că corpul senzorului acoperă găurile din dreapta (g-j).

Pune DHT11 pe rândurile **40-42**, cu pinii în coloana f:

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
40 │ ○ ○ ○ ○ ○   GND ○ ○ ○ ○ │  ← Pin GND în (40, f)
41 │ ○ ○ ○ ○ ○   DATA○ ○ ○ ○ │  ← Pin DATA în (41, f)
42 │ ○ ○ ○ ○ ○   VCC ○ ○ ○ ○ │  ← Pin VCC în (42, f)
   └─────────────────────────┘
```

### 6.2 Conexiuni DHT11

| Rând | Pin DHT11 | Gaură liberă | Conectează la |
|------|-----------|--------------|---------------|
| 40 | GND în (40,f) | **(40, g)** | Fir la linia (-) GND |
| 41 | DATA în (41,f) | **(41, g)** | Fir la FRDM.PTD4 |
| 42 | VCC în (42,f) | **(42, g)** | Vine de la LDR (daisy-chain) |
|    |           | **(42, h)** | Fir daisy-chain la Bluetooth (16, h) |

---

## PASUL 7: Montează LDR (Fotorezistor) și LED

### 7.1 LDR cu Rezistor Pull-Down

> [!IMPORTANT]
> LDR are 2 picioare - pune-le **vertical** în 2 rânduri diferite!

**Poziționare pe rândurile 60-62:**

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
60 │ ○ ○ ○ ○ ○   L ○ ○ ○ ● │  ← LDR picior 1 în (60, f) + fir 3.3V în (60, j)
61 │ ○ ○ ○ ○ ○   L ○ R ○ ● │  ← LDR picior 2 în (61, f) + Rezistor 10kΩ în (61, h) + fir PTB0 în (61, j)
62 │ ○ ○ ○ ○ ○   ○ ○ R ○ ● │  ← Rezistor 10kΩ în (62, h) + fir GND în (62, j)
   └─────────────────────────┘
```

**Conexiuni pas cu pas:**

1. **LDR**: Un picior în **(60, f)**, celălalt în **(61, f)**
2. **Fir 3.3V de la FRDM**: De la FRDM.3V3 la **(60, j)** - alimentează LDR
3. **Rezistor 10kΩ**: Un picior în **(61, h)**, celălalt în **(62, h)**
4. **Fir la PTB0**: De la **(61, j)** la FRDM.PTB0 - citire ADC
5. **Fir GND**: De la **(62, j)** la linia (-) GND

**Schema electrică:**
```
3.3V ─── LDR ───┬─── PTB0 (citire)
                │
             10kΩ
                │
               GND
```

### 7.2 LED-uri Faruri (2 LED-uri)

> [!IMPORTANT]
> LED-urile se pun **vertical** - anodul și catodul în rânduri diferite!

**Poziționare pe rândurile 55-59:**

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
55 │ L+ ● L+ ○ ○   ○ ○ ○ ○ ○ │  ← LED1 anod (55,a), fir PTC1 (55,b), LED2 anod (55,c)
56 │ L- ● L- ● ○   ○ ○ ○ ○ ○ │  ← LED1 catod (56,a) + R1 (56,b), LED2 catod (56,c) + R2 (56,d)
57 │ ○  ○ ○  ○ ○   ○ ○ ○ ○ ○ │
58 │ ○  ○ ○  ○ ○   ○ ○ ○ ○ ○ │
59 │ G  ● G  ● ○   ○ ○ ○ ○ ○ │  ← R1 (59,b) + GND (59,a), R2 (59,d) + GND (59,c)
   └─────────────────────────┘
```

**Conexiuni pas cu pas pentru LED1 (far stâng):**

1. **LED1**: Picior LUNG (anod) în **(55, a)**, picior SCURT (catod) în **(56, a)**
2. **Rezistor 330Ω (R1)**: Un picior în **(56, b)**, celălalt în **(59, b)**
3. **Fir GND**: De la **(59, a)** la linia (-) GND

**Conexiuni pas cu pas pentru LED2 (far drept):**

4. **LED2**: Picior LUNG (anod) în **(55, c)**, picior SCURT (catod) în **(56, c)**
5. **Rezistor 330Ω (R2)**: Un picior în **(56, d)**, celălalt în **(59, d)**
6. **Fir GND**: De la **(59, c)** la linia (-) GND

**Conexiunea de control:**

7. **Fir de la FRDM.PTC1**: La **(55, b)** 
   - Controlează ambele LED-uri automat! (55,a), (55,b), (55,c) sunt pe același rând, deci conectate.
   - Nu ai nevoie de jumper suplimentar!

**Schema electrică:**
```
              ┌───[330Ω]───LED1(-)───GND
PTC1 ────────┤
              └───[330Ω]───LED2(-)───GND
```

> [!TIP]
> **Ambele LED-uri se aprind și se sting simultan!** Când PTC1 e HIGH, ambele faruri sunt aprinse.

---

## PASUL 8: Montează Modulul Bluetooth HC-05/HC-06

Modulul Bluetooth are 6 pini. Folosim doar 4: VCC, GND, TXD, RXD (sărim EN și STATE).

> [!IMPORTANT]
> Modulul Bluetooth necesită **5V** pentru alimentare, dar pinii de date (TXD) trimit 5V!
> Trebuie un **divizor de tensiune** pe linia TXD → PTA1 pentru a proteja FRDM-ul (3.3V).

### 8.1 Poziționare și Divizor de Tensiune

```
     a b c d e   f g h i j
   ┌─────────────────────────┐
15 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │  ← gol (skip EN)
16 │ ○ ○ ○ ○ ○   VCC ○ ○ ○ ○│  ← Pin VCC în (16, f) → la FRDM.5V
17 │ ○ ○ ○ ○ ○   GND ○ ○ ○ ○│  ← Pin GND în (17, f) → la Linia (-)
18 │ ○ ○ ○ ○ ○   TXD ● ○ ○ ○│  ← Pin TXD în (18, f), rezistor 1kΩ (18,g)→(22,g)
19 │ ○ ○ ○ ○ ○   RXD ● ○ ○ ○│  ← Pin RXD în (19, f) → direct la PTA2
20 │ ○ ○ ○ ○ ○   ○ ○ ○ ○ ○ │  ← gol (skip STATE)
   │ ... │
22 │ ○ ○ ○ ○ ○   ○ ● ● ○ ● │  ← Rezistor 1kΩ (22,g) + 2kΩ (22,h) + PTA1 (22,j)
   │ ... │
26 │ ○ ○ ○ ○ ○   ○ ○ ● ● ○ │  ← Rezistor 2kΩ (26,h) + GND (26,i)
   └─────────────────────────┘
```

### 8.2 Conexiuni Bluetooth

| Componentă | De la | La | Notă |
|------------|-------|-----|------|
| **VCC** | (16, f) | **FRDM.P5V_USB** (J9) | Alimentare 5V |
| **GND** | (17, g) | **Linia (-)** | Masă comună |
| **RXD** | (19, g) | **FRDM.PTA2** (J1) | Direct - FRDM trimite 3.3V |
| **TXD** | (18, f) | Divizor (vezi mai jos) | NU direct la PTA1! |

### 8.3 Divizor de Tensiune pentru TXD (OBLIGATORIU!)

> [!CAUTION]
> **TXD trimite 5V! FRDM.PTA1 suportă doar 3.3V!**
> Fără divizor, poți arde pinul PTA1!

**Schema:**
```
Bluetooth TXD (18,f) ───[1kΩ]───┬─── FRDM.PTA1
                                │
                             [2kΩ]
                                │
                               GND
```

**Conexiuni pas cu pas:**

1. **Rezistor 1kΩ**: Un picior în **(18, g)**, celălalt în **(22, g)**
2. **Rezistor 2kΩ**: Un picior în **(22, h)**, celălalt în **(26, h)**
3. **Fir GND**: De la **(26, i)** la linia (-) GND
4. **Fir PTA1**: De la **(22, j)** la **FRDM.PTA1** (J1)

> [!TIP]
> Punctul (22, g), (22, h) și (22, j) sunt pe același rând, deci sunt **conectate electric**.
> Formula: V_out = 5V × (2kΩ / (1kΩ + 2kΩ)) = **3.3V** ✅

> [!WARNING]
> **TXD al Bluetooth-ului merge la RX-ul plăcii (PTA1) și invers!**

---

## PASUL 9: Conectează Totul la FRDM-KL25Z

Acum conectăm toate firele de la breadboard la placa FRDM-KL25Z.

### 9.1 Localizează pinii pe FRDM-KL25Z

Placa are 2 headere principale (J1 și J2) pe margini.

### 9.2 Lista completă de conexiuni FRDM ↔ Breadboard

| Pin FRDM | Header | Conectează la | Descriere |
|----------|--------|---------------|-----------|
| **GND** | J9 | Linia (-) breadboard | Masă comună |
| **3V3** | J9 | DHT11 VCC, Bluetooth VCC, LDR, L293D Pin16 | Alimentare 3.3V |
| **VIN** | J9 | Linia (+) breadboard | Alimentare placă de la baterii |
| **PTA1** | J1 | Bluetooth TXD | UART RX |
| **PTA2** | J1 | Bluetooth RXD | UART TX |
| **PTA4** | J1 | L293D Pin 1 (EN1) = (5, a) | PWM Motor Stânga |
| **PTA5** | J1 | L293D Pin 9 (EN2) = (12, h) | PWM Motor Dreapta |
| **PTB0** | J10 | LDR punct mijloc (61, j) | ADC citire lumină |
| **PTB1** | J10 | L293D Pin 2 (IN1) = (6, a) | Motor Stânga IN1 |
| **PTB2** | J10 | L293D Pin 7 (IN2) = (11, a) | Motor Stânga IN2 |
| **PTB3** | J10 | L293D Pin 10 (IN3) = (11, h) | Motor Dreapta IN1 |
| **PTC1** | J10 | LED1+LED2 anod (55, b) | Control 2 faruri |
| **PTC2** | J10 | L293D Pin 15 (IN4) = (6, h) | Motor Dreapta IN2 |
| **PTC8** | J1 | HC-SR04 TRIG (31, b) | Ultrasonic trigger |
| **PTC9** | J1 | Divizor ECHO (34, b) | Ultrasonic echo |
| **PTD4** | J1 | DHT11 DATA (41, g) | Senzor temperatură |

### 9.3 Alimentare Autonomă (fără laptop)

Pentru ca mașinuța să funcționeze fără laptop:

| De la | La |
|-------|-----|
| Linia (+) breadboard (6V baterii) | FRDM.VIN (pe J9) |
| Linia (-) breadboard | FRDM.GND (pe J9) |

**Când switch-ul de pe mașinuță e ON:**
- Bateriile alimentează L293D (motoare)
- Bateriile alimentează FRDM-KL25Z (prin VIN)
- Totul funcționează autonom! 🎉

---

## PASUL 10: Verificare Finală

### 10.1 Checklist înainte de pornire

- [ ] Switch baterii pe OFF
- [ ] L293D poziționat corect (crestătura sus)
- [ ] Toate 4 GND-urile L293D conectate
- [ ] Divizor de tensiune pe ECHO (1kΩ + 2kΩ)
- [ ] Bluetooth la 3.3V (NU 5V)
- [ ] 2x LED-uri, fiecare cu rezistor 330Ω propriu
- [ ] LED-uri pe rândurile 55-59, fiecare cu rezistor 330Ω
- [ ] LDR cu rezistor 10kΩ la GND
- [ ] Firele motoarelor la OUT1/OUT2 și OUT3/OUT4
- [ ] VIN conectat la linia (+) pentru alimentare autonomă
- [ ] GND FRDM conectat la linia (-)

### 10.2 Test inițial

1. **Lasă switch-ul pe OFF**
2. Conectează FRDM la laptop prin USB
3. Încarcă programul
4. Deconectează USB
5. Pune switch-ul pe ON
6. LED-ul de pe FRDM ar trebui să se aprindă

### 10.3 Test Bluetooth

1. Instalează "Serial Bluetooth Terminal" pe telefon
2. Împerechează cu "HC-05" sau "HC-06" (cod: 1234 sau 0000)
3. Conectează-te din aplicație
4. Trimite "I" pentru a vedea informațiile senzorilor
5. Trimite "F" pentru a merge înainte

---

## Diagrama Completă

```
═══════════════════════════════════════════════════════════════════════════
                    VEDERE DE SUS A BREADBOARD-ULUI
═══════════════════════════════════════════════════════════════════════════

  (+) ════════════════════════════════════════════════════════════ (+) 6V
  (-) ════════════════════════════════════════════════════════════ (-) GND
  
     a   b   c   d   e       f   g   h   i   j
   ┌─────────────────────────────────────────────┐
 1 │                                             │
 2 │                                             │
 3 │                                             │
 4 │                                             │
 5 │ ○   ○   ○   ○  [EN1]   [VCC1] ○   ○   ○   ○ │ ← L293D Pin 1, 16
 6 │ ○   ○   ○   ○  [IN1]   [IN4]  ○   ○   ○   ○ │ ← L293D Pin 2, 15
 7 │ M←  M←  ○   ○  [OUT1]  [OUT4] ○   ○   →M  →M│ ← Motoare
 8 │ ○   ○   ○   ○  [GND]   [GND]  ○   ○   ○   ○ │
 9 │ ○   ○   ○   ○  [GND]   [GND]  ○   ○   ○   ○ │
10 │ M←  M←  ○   ○  [OUT2]  [OUT3] ○   ○   →M  →M│ ← Motoare
11 │ ○   ○   ○   ○  [IN2]   [IN3]  ○   ○   ○   ○ │ ← L293D Pin 7, 10
12 │ ○   ○   ○   ○  [VCC2]  [EN2]  ○   ○   ○   ○ │ ← L293D Pin 8, 9
13 │                                             │
14 │                                             │
15 │[V] [G] [T] [R]  ○       ○   ○   ○   ○   ○   │ ← Bluetooth HC-05
16 │[C] [N] [X] [X]  ○       ○   ○   ○   ○   ○   │
17 │[C] [D] [D] [D]  ○       ○   ○   ○   ○   ○   │
    │                                             │
    │ ... rânduri libere 18-29 ...                │
    │                                             │
30 │[VCC] ○   ○   ○   ○      ○   ○   ○   ○   ○   │ ← HC-SR04 (vertical)
31 │[TRG] ○   ○   ○   ○      ○   ○   ○   ○   ○   │   pe coloana a
32 │[ECH] ○  [1k] ○   ○      ○   ○   ○   ○   ○   │ ← Rezistor 1k
33 │[GND] ○   ○   ○   ○      ○   ○   ○   ○   ○   │
34 │ ○   ○  [•]  ○   ○      ○   ○   ○   ○   ○   │ ← Punct PTC9
35 │ ○   ○  [2k] ○   ○      ○   ○   ○   ○   ○   │ ← Rezistor 2k→GND
    │                                             │
    │ ... rânduri libere 36-39 ...                │
    │                                             │
40 │ ○   ○   ○   ○   ○      [V] [D] [G]  ○   ○   │ ← DHT11
    │                                             │
    │ ... rânduri libere 41-44 ...                │
    │                                             │
45 │ ○   ○   ○   ○   ○      [L] [L]  ○   ○   ○   │ ← LDR pe (45,f-g)
46 │ ○   ○   ○   ○   ○      [10k]   →PTB0        │ ← Rezistor 10k
47 │ ○   ○   ○   ○   ○      [to GND]              │
    │                                             │
    │ ... rânduri libere 48-49 ...                │
    │                                             │
50 │[L1+][L1-][L2+][L2-]○   ○   ○   ○   ○   ○   │ ← 2x LED pe (50,a-d)
51 │[R1]     [R2]     ○   ○   ○   ○   ○   ○   │ ← Rezistori 330Ω
52 │[GND]    [GND]    ○   ○   ○   ○   ○   ○   │
   └─────────────────────────────────────────────┘

  (+) ════════════════════════════════════════════════════════════ (+)
  (-) ════════════════════════════════════════════════════════════ (-)

LEGENDĂ:
  [EN1] = Pin L293D
  M← = Fir la motor stânga
  →M = Fir la motor dreapta
  [L] = LDR
  [LED+] = LED anod
  [1k], [2k], [10k], [330Ω] = Rezistori
```

---

## Rezumat Fire FRDM → Breadboard

| # | Din FRDM | Către Breadboard | Culoare sugerată |
|---|----------|------------------|------------------|
| 1 | GND (J9) | Linia (-) | Negru |
| 2 | 3V3 (J9) | LDR (60, j) | Roșu |
| 3 | VIN (J9) | Linia (+) | Roșu gros |
| 4 | P5V_USB (J9) | Bluetooth VCC (direct) | Roșu |
| 5 | PTA1 (J1) | Bluetooth TXD prin divizor (22, j) | Galben |
| 6 | PTA2 (J1) | Bluetooth RXD (19, g) direct | Verde |
| 6 | PTA4 | L293D EN1 (5, a) | Albastru |
| 7 | PTA5 | L293D EN2 (12, h) | Albastru |
| 8 | PTB0 | LDR mijloc (61, j) | Portocaliu |
| 9 | PTB1 | L293D IN1 (6, a) | Mov |
| 10 | PTB2 | L293D IN2 (11, a) | Mov |
| 11 | PTB3 | L293D IN3 (11, h) | Maro |
| 12 | PTC1 | LED (55, b) | Alb |
| 13 | PTC2 | L293D IN4 (6, h) | Maro |
| 14 | PTC8 | HC-SR04 TRIG (31, b) | Galben |
| 15 | PTC9 | Divizor ECHO (34, b) | Portocaliu |
| 16 | PTD4 | DHT11 DATA (41, g) | Verde |

---

## Debugging Probleme Comune

| Simptom | Verifică |
|---------|----------|
| Motoarele nu merg deloc | GND baterii conectat la GND FRDM? |
| Un motor merge, celălalt nu | Fire motor bine înfipte? L293D alimentat? |
| Motoare merg invers | Inversează firele motorului respectiv |
| HC-SR04 returnează 500cm mereu | Verifică divizorul ECHO, verifică alimentarea |
| DHT11 eroare | Verifică VCC la 3.3V, DATA la PTD4 |
| Bluetooth nu se conectează | VCC la 3.3V? LED Bluetooth clipește? |
| Un LED nu se aprinde | Verifică polaritatea, rezistor de 330Ω present? |
| Ambele LED-uri nu merg | Verifică firul de la PTC1 la (27, f) |
| FRDM nu pornește de pe baterii | VIN conectat la (+)? GND la (-)? |
