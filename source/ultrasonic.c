#include "ultrasonic.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_clock.h"
#include "MKL25Z4.h"
#include "uart.h"

/**
 * HC-SR04 Ultrasonic Distance Sensor
 * 
 * Working Principle:
 * 1. Send 10µs HIGH pulse on TRIG
 * 2. Sensor sends 8x 40kHz ultrasonic bursts
 * 3. ECHO pin goes HIGH
 * 4. When echo returns, ECHO pin goes LOW
 * 5. Distance = (ECHO pulse duration in µs) / 58
 * 
 * Speed of sound: 343 m/s = 0.0343 cm/µs
 * Round trip: 0.0343 / 2 = 0.01715 cm/µs
 * So: distance_cm = time_µs * 0.01715 ≈ time_µs / 58
 * 
 * TIMING IMPLEMENTATION:
 * Uses TPM2 as a free-running counter for precise timing.
 * TPM2 config: 48MHz / 32 (prescaler) = 1.5MHz = 0.667µs per tick
 * This provides hardware-based timing independent of interrupts.
 */

// Pin definitions - Port C (J1 header)
#define ULTRASONIC_TRIG_GPIO    GPIOC
#define ULTRASONIC_TRIG_PORT    PORTC
#define ULTRASONIC_TRIG_PIN     8U          // PTC8 (J1)

#define ULTRASONIC_ECHO_GPIO    GPIOC
#define ULTRASONIC_ECHO_PORT    PORTC
#define ULTRASONIC_ECHO_PIN     9U          // PTC9 (J1)

// Timer configuration (TPM2 @ 48MHz with prescaler 32 = 1.5MHz)
// 1 tick = 0.667µs, overflow at 65535 ticks = ~43.7ms
#define TPM2_PRESCALER          5U          // PS=5 means divide by 32
#define TPM2_FREQ_HZ            1500000U    // 48MHz / 32 = 1.5MHz
#define TICKS_PER_US_NUM        3U          // 1.5 ticks per µs (numerator)
#define TICKS_PER_US_DEN        2U          // 1.5 ticks per µs (denominator)

// Timeout: 30ms = 30000µs * 1.5 = 45000 ticks
#define TIMEOUT_TICKS           45000U
// For shorter waits (e.g., waiting for ECHO to go HIGH after TRIG)
#define SHORT_TIMEOUT_TICKS     15000U      // ~10ms

/**
 * @brief Read the current TPM2 counter value
 */
static inline uint16_t Ultrasonic_GetTicks(void)
{
    return (uint16_t)(TPM2->CNT & 0xFFFF);
}

/**
 * @brief Calculate elapsed ticks handling 16-bit overflow
 */
static inline uint16_t Ultrasonic_ElapsedTicks(uint16_t start, uint16_t end)
{
    return (uint16_t)(end - start);  // Works correctly with 16-bit wrap-around
}

/**
 * @brief Convert ticks to microseconds
 * Formula: µs = ticks * 2 / 3 (since 1.5 ticks = 1µs)
 */
static inline uint32_t Ultrasonic_TicksToUs(uint16_t ticks)
{
    return ((uint32_t)ticks * TICKS_PER_US_DEN) / TICKS_PER_US_NUM;
}

/**
 * @brief Convert microseconds to ticks
 * Formula: ticks = µs * 3 / 2 (since 1µs = 1.5 ticks)
 */
static inline uint16_t Ultrasonic_UsToTicks(uint32_t us)
{
    uint32_t ticks = (us * TICKS_PER_US_NUM) / TICKS_PER_US_DEN;
    if (ticks > 0xFFFF) ticks = 0xFFFF;
    return (uint16_t)ticks;
}

/**
 * @brief Microsecond delay using TPM2 hardware timer
 * Much more precise than NOP-based delay
 */
static void Ultrasonic_DelayUs(uint32_t us)
{
    uint16_t startTicks = Ultrasonic_GetTicks();
    uint16_t delayTicks = Ultrasonic_UsToTicks(us);
    
    while (Ultrasonic_ElapsedTicks(startTicks, Ultrasonic_GetTicks()) < delayTicks) {
        // Busy wait using hardware timer
    }
}

/**
 * @brief Read ECHO pin state
 */
static inline uint8_t Ultrasonic_ReadEcho(void)
{
    return (ULTRASONIC_ECHO_GPIO->PDIR & (1U << ULTRASONIC_ECHO_PIN)) ? 1 : 0;
}

/**
 * @brief Set TRIG pin state
 */
static inline void Ultrasonic_SetTrig(uint8_t value)
{
    if (value) {
        ULTRASONIC_TRIG_GPIO->PSOR = (1U << ULTRASONIC_TRIG_PIN);
    } else {
        ULTRASONIC_TRIG_GPIO->PCOR = (1U << ULTRASONIC_TRIG_PIN);
    }
}

/**
 * @brief Initialize TPM2 as free-running counter for timing
 */
static void Ultrasonic_InitTimer(void)
{
    // Enable TPM2 clock gate
    SIM->SCGC6 |= SIM_SCGC6_TPM2_MASK;
    
    // TPM clock source should already be set to MCGFLLCLK in motor.c
    // But set it here too in case ultrasonic is initialized first
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);
    
    // Stop TPM2 before configuring
    TPM2->SC = 0;
    
    // Reset counter
    TPM2->CNT = 0;
    
    // Set prescaler to divide by 32 (PS=5)
    // 48MHz / 32 = 1.5MHz = 0.667µs per tick
    TPM2->SC = TPM_SC_PS(TPM2_PRESCALER);
    
    // Set MOD to maximum for free-running mode
    TPM2->MOD = 0xFFFF;
    
    // Start TPM2 with internal clock
    TPM2->SC |= TPM_SC_CMOD(1);
    
    UART_SendString("  TPM2 timer configured (1.5MHz)\r\n");
}

void Ultrasonic_Init(void)
{
    gpio_pin_config_t trigConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    
    gpio_pin_config_t echoConfig = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U
    };
    
    // Initialize TPM2 timer first (needed for delays)
    Ultrasonic_InitTimer();
    
    // Enable Port C clock (for PTC8 and PTC9)
    CLOCK_EnableClock(kCLOCK_PortC);
    
    // Configure TRIG pin (PTC8) as output
    PORT_SetPinMux(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(ULTRASONIC_TRIG_GPIO, ULTRASONIC_TRIG_PIN, &trigConfig);
    
    // Configure ECHO pin (PTC9) as input
    PORT_SetPinMux(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(ULTRASONIC_ECHO_GPIO, ULTRASONIC_ECHO_PIN, &echoConfig);
    
    // Ensure TRIG starts LOW
    Ultrasonic_SetTrig(0);
    
    // Wait for sensor to stabilize (50ms)
    Ultrasonic_DelayUs(50000);

    UART_SendString("  ULTRASONIC init finish\r\n");
}

uint32_t Ultrasonic_GetPulseUs(void)
{
    uint16_t startTicks, endTicks, waitStart;
    uint16_t pulseTicks;
    
    // Debug: check initial ECHO state
    if (Ultrasonic_ReadEcho() == 1) {
        UART_SendString("[US] Warning: ECHO is HIGH before trigger!\r\n");
        // Wait for ECHO to go LOW with timeout
        waitStart = Ultrasonic_GetTicks();
        while (Ultrasonic_ReadEcho() == 1) {
            if (Ultrasonic_ElapsedTicks(waitStart, Ultrasonic_GetTicks()) > SHORT_TIMEOUT_TICKS) {
                UART_SendString("[US] Error: ECHO stuck HIGH, cannot measure\r\n");
                return 0;
            }
        }
    }
    
    // Ensure TRIG is LOW before starting
    Ultrasonic_SetTrig(0);
    Ultrasonic_DelayUs(2);
    
    // Send 10µs trigger pulse (hardware-timed, precise)
    Ultrasonic_SetTrig(1);
    Ultrasonic_DelayUs(10);
    Ultrasonic_SetTrig(0);
    
    // Wait for ECHO to go HIGH (with timeout)
    waitStart = Ultrasonic_GetTicks();
    while (Ultrasonic_ReadEcho() == 0) {
        if (Ultrasonic_ElapsedTicks(waitStart, Ultrasonic_GetTicks()) > SHORT_TIMEOUT_TICKS) {
            UART_SendString("[US] Timeout: ECHO never went HIGH\r\n");
            return 0;
        }
    }
    
    // Capture start time immediately when ECHO goes HIGH
    startTicks = Ultrasonic_GetTicks();
    
    // Wait for ECHO to go LOW (measure pulse duration)
    while (Ultrasonic_ReadEcho() == 1) {
        if (Ultrasonic_ElapsedTicks(startTicks, Ultrasonic_GetTicks()) > TIMEOUT_TICKS) {
            UART_SendString("[US] Timeout: ECHO stuck HIGH\r\n");
            return 0;
        }
    }
    
    // Capture end time immediately when ECHO goes LOW
    endTicks = Ultrasonic_GetTicks();
    
    // Calculate pulse width in ticks
    pulseTicks = Ultrasonic_ElapsedTicks(startTicks, endTicks);
    
    // Convert to microseconds and return
    return Ultrasonic_TicksToUs(pulseTicks);
}

uint32_t Ultrasonic_GetDistanceCm(void)
{
    uint32_t pulseUs = Ultrasonic_GetPulseUs();
    
    if (pulseUs == 0) {
        return ULTRASONIC_TIMEOUT_CM;  // Error/timeout
    }
    
    // Distance = pulse_µs / 58
    uint32_t distanceCm = pulseUs / 58;
    
    // Clamp to valid range
    if (distanceCm < ULTRASONIC_MIN_DISTANCE_CM) {
        return ULTRASONIC_MIN_DISTANCE_CM;
    }
    if (distanceCm > ULTRASONIC_MAX_DISTANCE_CM) {
        return ULTRASONIC_TIMEOUT_CM;
    }
    
    return distanceCm;
}

bool Ultrasonic_ObstacleDetected(uint32_t thresholdCm)
{
    uint32_t distance = Ultrasonic_GetDistanceCm();
    
    // If we got a valid reading within threshold, obstacle detected
    if (distance < ULTRASONIC_TIMEOUT_CM && distance <= thresholdCm) {
        return true;
    }
    
    return false;
}
