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
 * UPDATED PIN ASSIGNMENTS (for available header pins):
 *   - TRIG: PTC8 (J1)
 *   - ECHO: PTC9 (J1) - USE VOLTAGE DIVIDER!
 */

// Pin definitions - Port C (J1 header)
#define ULTRASONIC_TRIG_GPIO    GPIOC
#define ULTRASONIC_TRIG_PORT    PORTC
#define ULTRASONIC_TRIG_PIN     8U          // PTC8 (J1)

#define ULTRASONIC_ECHO_GPIO    GPIOC
#define ULTRASONIC_ECHO_PORT    PORTC
#define ULTRASONIC_ECHO_PIN     9U          // PTC9 (J1)

// Timing constants (at 48MHz core clock)
#define CYCLES_PER_US           48U
#define TIMEOUT_US              30000U  // 30ms timeout (~5m max distance)

/**
 * @brief Microsecond delay using busy loop
 */
static void Ultrasonic_DelayUs(uint32_t us)
{
    volatile uint32_t cycles = us * 16;  // Approximate for 48MHz
    while (cycles--) {
        __asm("nop");
        __asm("nop");
        __asm("nop");
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
    
    // Wait for sensor to stabilize
    Ultrasonic_DelayUs(50000);  // 50ms

    UART_SendString("  ULTRASONIC init finish  \r\n");
}

uint32_t Ultrasonic_GetPulseUs(void)
{
    uint32_t timeout;
    uint32_t pulseWidth = 0;
    
    // Debug: check initial ECHO state
    if (Ultrasonic_ReadEcho() == 1) {
        UART_SendString("[US] Warning: ECHO is HIGH before trigger!\r\n");
    }
    
    // Ensure TRIG is LOW before starting
    Ultrasonic_SetTrig(0);
    Ultrasonic_DelayUs(2);
    
    // Send 10µs trigger pulse
    Ultrasonic_SetTrig(1);
    Ultrasonic_DelayUs(10);
    Ultrasonic_SetTrig(0);
    
    // Wait for ECHO to go HIGH (with timeout)
    timeout = TIMEOUT_US;
    while (Ultrasonic_ReadEcho() == 0) {
        Ultrasonic_DelayUs(1);
        if (--timeout == 0) {
            UART_SendString("[US] Timeout: ECHO never went HIGH\r\n");
            return 0;  // Timeout waiting for echo start
        }
    }
    
    // Measure ECHO pulse width (time it stays HIGH)
    timeout = TIMEOUT_US;
    while (Ultrasonic_ReadEcho() == 1) {
        Ultrasonic_DelayUs(1);
        pulseWidth++;
        if (--timeout == 0) {
            UART_SendString("[US] Timeout: ECHO stuck HIGH\r\n");
            return 0;  // Timeout waiting for echo end
        }
    }
    
    return pulseWidth;
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
