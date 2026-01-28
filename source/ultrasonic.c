#include "ultrasonic.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_clock.h"
#include "MKL25Z4.h"
#include "uart.h"

/**
 * Dual HC-SR04 Ultrasonic Distance Sensors
 * 
 * Working Principle:
 * 1. Send 10µs HIGH pulse on TRIG (shared for both sensors)
 * 2. Sensor sends 8x 40kHz ultrasonic bursts
 * 3. ECHO pin goes HIGH
 * 4. When echo returns, ECHO pin goes LOW
 * 5. Distance = (ECHO pulse duration in µs) / 58
 * 
 * TIMING IMPLEMENTATION:
 * Uses TPM2 as a free-running counter for precise timing.
 * TPM2 config: 48MHz / 32 (prescaler) = 1.5MHz = 0.667µs per tick
 */

// Shared TRIG pin (PTC8)
#define ULTRASONIC_TRIG_GPIO    GPIOC
#define ULTRASONIC_TRIG_PORT    PORTC
#define ULTRASONIC_TRIG_PIN     8U          // PTC8 (J1)

// FRONT sensor ECHO pin (PTC9)
#define ULTRASONIC_ECHO_FRONT_GPIO    GPIOC
#define ULTRASONIC_ECHO_FRONT_PORT    PORTC
#define ULTRASONIC_ECHO_FRONT_PIN     9U    // PTC9 (J1)

// REAR sensor ECHO pin (PTA12)
#define ULTRASONIC_ECHO_REAR_GPIO     GPIOA
#define ULTRASONIC_ECHO_REAR_PORT     PORTA
#define ULTRASONIC_ECHO_REAR_PIN      12U   // PTA12

// Timer configuration (TPM2 @ 48MHz with prescaler 32 = 1.5MHz)
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
    return (uint16_t)(end - start);
}

/**
 * @brief Convert ticks to microseconds
 */
static inline uint32_t Ultrasonic_TicksToUs(uint16_t ticks)
{
    return ((uint32_t)ticks * TICKS_PER_US_DEN) / TICKS_PER_US_NUM;
}

/**
 * @brief Convert microseconds to ticks
 */
static inline uint16_t Ultrasonic_UsToTicks(uint32_t us)
{
    uint32_t ticks = (us * TICKS_PER_US_NUM) / TICKS_PER_US_DEN;
    if (ticks > 0xFFFF) ticks = 0xFFFF;
    return (uint16_t)ticks;
}

/**
 * @brief Microsecond delay using TPM2 hardware timer
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
 * @brief Read ECHO pin state for specified sensor
 */
static inline uint8_t Ultrasonic_ReadEcho(UltrasonicSensor_t sensor)
{
    if (sensor == ULTRASONIC_FRONT) {
        return (ULTRASONIC_ECHO_FRONT_GPIO->PDIR & (1U << ULTRASONIC_ECHO_FRONT_PIN)) ? 1 : 0;
    } else {
        return (ULTRASONIC_ECHO_REAR_GPIO->PDIR & (1U << ULTRASONIC_ECHO_REAR_PIN)) ? 1 : 0;
    }
}

/**
 * @brief Set TRIG pin state (shared for both sensors)
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
    
    // TPM clock source
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);
    
    // Stop TPM2 before configuring
    TPM2->SC = 0;
    TPM2->CNT = 0;
    
    // Set prescaler to divide by 32 (PS=5)
    TPM2->SC = TPM_SC_PS(TPM2_PRESCALER);
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
    
    // Initialize TPM2 timer first
    Ultrasonic_InitTimer();
    
    // Enable Port clocks
    CLOCK_EnableClock(kCLOCK_PortA);  // For PTA12 (rear ECHO)
    CLOCK_EnableClock(kCLOCK_PortC);  // For PTC8 (TRIG) and PTC9 (front ECHO)
    
    // Configure shared TRIG pin (PTC8) as output
    PORT_SetPinMux(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(ULTRASONIC_TRIG_GPIO, ULTRASONIC_TRIG_PIN, &trigConfig);
    
    // Configure FRONT ECHO pin (PTC9) as input
    PORT_SetPinMux(ULTRASONIC_ECHO_FRONT_PORT, ULTRASONIC_ECHO_FRONT_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(ULTRASONIC_ECHO_FRONT_GPIO, ULTRASONIC_ECHO_FRONT_PIN, &echoConfig);
    
    // Configure REAR ECHO pin (PTA12) as input
    PORT_SetPinMux(ULTRASONIC_ECHO_REAR_PORT, ULTRASONIC_ECHO_REAR_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(ULTRASONIC_ECHO_REAR_GPIO, ULTRASONIC_ECHO_REAR_PIN, &echoConfig);
    
    // Ensure TRIG starts LOW
    Ultrasonic_SetTrig(0);
    
    // Wait for sensors to stabilize (50ms)
    Ultrasonic_DelayUs(50000);

    UART_SendString("  ULTRASONIC (DUAL) init finish\r\n");
    UART_SendString("    FRONT: TRIG=PTC8, ECHO=PTC9\r\n");
    UART_SendString("    REAR:  TRIG=PTC8, ECHO=PTA12\r\n");
}

uint32_t Ultrasonic_GetPulseUs(UltrasonicSensor_t sensor)
{
    uint16_t startTicks, endTicks, waitStart;
    uint16_t pulseTicks;
    
    // Check initial ECHO state
    if (Ultrasonic_ReadEcho(sensor) == 1) {
        // Wait for ECHO to go LOW with timeout
        waitStart = Ultrasonic_GetTicks();
        while (Ultrasonic_ReadEcho(sensor) == 1) {
            if (Ultrasonic_ElapsedTicks(waitStart, Ultrasonic_GetTicks()) > SHORT_TIMEOUT_TICKS) {
                return 0;
            }
        }
    }
    
    // Ensure TRIG is LOW before starting
    Ultrasonic_SetTrig(0);
    Ultrasonic_DelayUs(2);
    
    // Send 10µs trigger pulse
    Ultrasonic_SetTrig(1);
    Ultrasonic_DelayUs(10);
    Ultrasonic_SetTrig(0);
    
    // Wait for ECHO to go HIGH (with timeout)
    waitStart = Ultrasonic_GetTicks();
    while (Ultrasonic_ReadEcho(sensor) == 0) {
        if (Ultrasonic_ElapsedTicks(waitStart, Ultrasonic_GetTicks()) > SHORT_TIMEOUT_TICKS) {
            return 0;
        }
    }
    
    // Capture start time
    startTicks = Ultrasonic_GetTicks();
    
    // Wait for ECHO to go LOW
    while (Ultrasonic_ReadEcho(sensor) == 1) {
        if (Ultrasonic_ElapsedTicks(startTicks, Ultrasonic_GetTicks()) > TIMEOUT_TICKS) {
            return 0;
        }
    }
    
    // Capture end time
    endTicks = Ultrasonic_GetTicks();
    
    // Calculate pulse width
    pulseTicks = Ultrasonic_ElapsedTicks(startTicks, endTicks);
    
    return Ultrasonic_TicksToUs(pulseTicks);
}

uint32_t Ultrasonic_GetDistanceCm_Sensor(UltrasonicSensor_t sensor)
{
    uint32_t pulseUs = Ultrasonic_GetPulseUs(sensor);
    
    if (pulseUs == 0) {
        return ULTRASONIC_TIMEOUT_CM;
    }
    
    uint32_t distanceCm = pulseUs / 58;
    
    if (distanceCm < ULTRASONIC_MIN_DISTANCE_CM) {
        return ULTRASONIC_MIN_DISTANCE_CM;
    }
    if (distanceCm > ULTRASONIC_MAX_DISTANCE_CM) {
        return ULTRASONIC_TIMEOUT_CM;
    }
    
    return distanceCm;
}

// Backward compatible - reads FRONT sensor
uint32_t Ultrasonic_GetDistanceCm(void)
{
    return Ultrasonic_GetDistanceCm_Sensor(ULTRASONIC_FRONT);
}

// New function for REAR sensor
uint32_t Ultrasonic_GetRearDistanceCm(void)
{
    return Ultrasonic_GetDistanceCm_Sensor(ULTRASONIC_REAR);
}

bool Ultrasonic_ObstacleDetected(uint32_t thresholdCm)
{
    uint32_t distance = Ultrasonic_GetDistanceCm();
    
    if (distance < ULTRASONIC_TIMEOUT_CM && distance <= thresholdCm) {
        return true;
    }
    return false;
}

bool Ultrasonic_RearObstacleDetected(uint32_t thresholdCm)
{
    uint32_t distance = Ultrasonic_GetRearDistanceCm();
    
    if (distance < ULTRASONIC_TIMEOUT_CM && distance <= thresholdCm) {
        return true;
    }
    return false;
}
