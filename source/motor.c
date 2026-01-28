#include "motor.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_tpm.h"
#include "fsl_clock.h"
#include "MKL25Z4.h"
#include "uart.h"

/**
 * Motor Control using L293D Dual H-Bridge Driver
 * 
 * Pin Connections (UPDATED for available header pins):
 * Motor Left:
 *   - IN1: PTB1 (J10) - direction
 *   - IN2: PTB2 (J10) - direction
 *   - EN1: PTA4 (J1)  - PWM (TPM0_CH1)
 * 
 * Motor Right:
 *   - IN3: PTB3 (J10) - direction
 *   - IN4: PTC2 (J10) - direction
 *   - EN2: PTA5 (J1)  - PWM (TPM0_CH2)
 * 
 * Direction Logic:
 *   IN1=1, IN2=0 -> Forward
 *   IN1=0, IN2=1 -> Backward
 *   IN1=0, IN2=0 -> Stop (coast)
 *   IN1=1, IN2=1 -> Stop (brake)
 */

// Motor Left pins
#define MOTOR_L_IN1_GPIO    GPIOB
#define MOTOR_L_IN1_PORT    PORTB
#define MOTOR_L_IN1_PIN     1U          // PTB1 (J10)
#define MOTOR_L_IN2_GPIO    GPIOB
#define MOTOR_L_IN2_PORT    PORTB
#define MOTOR_L_IN2_PIN     2U          // PTB2 (J10)
#define MOTOR_L_EN_PORT     PORTA
#define MOTOR_L_EN_PIN      4U          // PTA4 = TPM0_CH1 (J1)

// Motor Right pins
#define MOTOR_R_IN1_GPIO    GPIOB
#define MOTOR_R_IN1_PORT    PORTB
#define MOTOR_R_IN1_PIN     3U          // PTB3 (J10)
#define MOTOR_R_IN2_GPIO    GPIOC
#define MOTOR_R_IN2_PORT    PORTC
#define MOTOR_R_IN2_PIN     2U          // PTC2 (J10)
#define MOTOR_R_EN_PORT     PORTA
#define MOTOR_R_EN_PIN      5U          // PTA5 = TPM0_CH2 (J1)

// PWM Configuration
#define PWM_FREQUENCY       1000U   // 1kHz PWM frequency

static uint8_t defaultSpeed = 100;   // Default speed 100%

// Compensare pentru motorul drept care e mai lent
#define RIGHT_MOTOR_BOOST  0 // +50% pentru motorul drept

/**
 * @brief Initialize TPM0 for PWM output
 */
static void Motor_InitPWM(void)
{
    UART_SendString("  Motor_InitPWM() start\r\n");
    
    // Enable TPM0 clock gate FIRST
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    
    // Set TPM clock source to PLLFLLCLK (which is MCGPLLCLK/2 = 48MHz in PEE mode)
    // TPMSRC = 1 means use PLLFLLCLK (configured by PLLFLLSEL in clock_config.c)
    // In BOARD_BootClockRUN, PLLFLLSEL is set to MCGPLLCLK, so we get 48MHz
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);
    
    // Use CLOCK_GetFreq with kCLOCK_PllFllSelClk to get the actual TPM source clock
    // This returns the PLLFLLCLK frequency which is what TPM actually uses
    uint32_t tpmClock = CLOCK_GetFreq(kCLOCK_PllFllSelClk);
    UART_SendString("  TPM Clock (PLLFLLCLK): ");
    UART_SendNumber(tpmClock);
    UART_SendString(" Hz\r\n");
    
    if (tpmClock == 0) {
        UART_SendString("  ERROR: TPM clock is 0! Using fallback.\r\n");
        tpmClock = 48000000U;  // Fallback to 48MHz
    }
    
    // Reset TPM0 (disable before configuration)
    TPM0->SC = 0;
    
    // Set prescaler to divide by 4
    TPM0->SC = TPM_SC_PS(2);  // PS=2 means divide by 4
    
    // Initialize TPM0: configure counter
    
    // Set up PWM for channel 1 (PTA4 - Motor Left)
    TPM0->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[1].CnV = 0;  // Start at 0% duty
    
    // Set up PWM for channel 2 (PTA5 - Motor Right)
    TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[2].CnV = 0;  // Start at 0% duty
    
    // Set MOD for 1kHz PWM (tpmClock / prescale / frequency)
    TPM0->MOD = (tpmClock / 4 / PWM_FREQUENCY) - 1;
    
    // Start TPM0
    TPM0->SC |= TPM_SC_CMOD(1);  // Use internal clock
    
    UART_SendString("  Motor_InitPWM() done\r\n");
}

static void Motor_SetBothPWM(uint8_t leftPercent, uint8_t rightPercent)
{
    if (leftPercent > 100) leftPercent = 100;
    if (rightPercent > 100) rightPercent = 100;
    
    uint32_t mod = TPM0->MOD;
    uint32_t leftDuty = (mod * leftPercent) / 100;
    uint32_t rightDuty = (mod * rightPercent) / 100;
    
    // Debug: print actual values
    UART_SendString("[PWM] MOD=");
    UART_SendNumber(mod);
    UART_SendString(" L_duty=");
    UART_SendNumber(leftDuty);
    UART_SendString(" R_duty=");
    UART_SendNumber(rightDuty);
    UART_SendString("\r\n");
    
    // SWAPPED: Hardware wiring has channels reversed
    TPM0->CONTROLS[1].CnV = rightDuty;  // TPM0_CH1 (PTA4) = Motor Right
    TPM0->CONTROLS[2].CnV = leftDuty;   // TPM0_CH2 (PTA5) = Motor Left
}

void Motor_Init(void)
{
    gpio_pin_config_t gpioConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    
    // Enable clocks for all ports used by motors
    CLOCK_EnableClock(kCLOCK_PortA);  // For PTA4, PTA5 (PWM)
    CLOCK_EnableClock(kCLOCK_PortB);  // For PTB1, PTB2, PTB3 (direction)
    CLOCK_EnableClock(kCLOCK_PortC);  // For PTC2 (direction)
    
    // Configure Motor Left direction pins (PTB1, PTB2) as GPIO
    PORT_SetPinMux(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, &gpioConfig);
    GPIO_PinInit(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, &gpioConfig);
    
    // Configure Motor Right direction pins (PTB3, PTC2) as GPIO
    PORT_SetPinMux(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, &gpioConfig);
    GPIO_PinInit(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, &gpioConfig);
    
    // Configure PWM pins
    // PTA4 = TPM0_CH1 (Alt3)
    PORT_SetPinMux(MOTOR_L_EN_PORT, MOTOR_L_EN_PIN, kPORT_MuxAlt3);
    // PTA5 = TPM0_CH2 (Alt3)
    PORT_SetPinMux(MOTOR_R_EN_PORT, MOTOR_R_EN_PIN, kPORT_MuxAlt3);
    
    UART_SendString("  MOTORS init before  Motor_InitPWM() \r\n");

    // Initialize PWM
    Motor_InitPWM();
    
    UART_SendString("  MOTORS init after  Motor_InitPWM() \r\n");

    // Start with motors stopped
    Motor_Stop();

    UART_SendString("  MOTORS init finish  \r\n");
}

void Motor_Forward(uint8_t speed)
{
    // Set directions: both motors FORWARD
    // Left: IN1=1, IN2=0 → Forward
    // Right: IN1=1, IN2=0 → Forward
    GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 1);
    GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 1);
    GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 0);
    
    // Compensate right motor (it's slower)
    uint8_t leftSpeed = (speed > RIGHT_MOTOR_BOOST) ? (speed - RIGHT_MOTOR_BOOST) : 0;
    
    UART_SendString("[FW] L=");
    UART_SendNumber(leftSpeed);
    UART_SendString(" R=");
    UART_SendNumber(speed);
    UART_SendString("\r\n");
    
    Motor_SetBothPWM(leftSpeed, speed);
}

void Motor_Backward(uint8_t speed)
{
    // Set directions: both motors BACKWARD
    // Left: IN1=0, IN2=1 → Backward
    // Right: IN1=0, IN2=1 → Backward
    GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 1);
    GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 1);
    
    uint8_t leftSpeed = (speed > RIGHT_MOTOR_BOOST) ? (speed - RIGHT_MOTOR_BOOST) : 0;
    
    UART_SendString("[BW] L=");
    UART_SendNumber(leftSpeed);
    UART_SendString(" R=");
    UART_SendNumber(speed);
    UART_SendString("\r\n");
    
    Motor_SetBothPWM(leftSpeed, speed);
}

void Motor_TurnLeft(uint8_t speed)
{
    // Pivot turn LEFT: left motor BACKWARD, right motor FORWARD
    // Left: IN1=0, IN2=1 → Backward
    // Right: IN1=1, IN2=0 → Forward
    GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 1);
    GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 1);
    GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 0);
    
    uint8_t leftSpeed = (speed > RIGHT_MOTOR_BOOST) ? (speed - RIGHT_MOTOR_BOOST) : 0;
    
    UART_SendString("[PIVOT LEFT] L=");
    UART_SendNumber(leftSpeed);
    UART_SendString(" R=");
    UART_SendNumber(speed);
    UART_SendString("\r\n");
    
    Motor_SetBothPWM(leftSpeed, speed);
}

void Motor_TurnRight(uint8_t speed)
{
    // Pivot turn RIGHT: left motor FORWARD, right motor BACKWARD
    // Left: IN1=1, IN2=0 → Forward
    // Right: IN1=0, IN2=1 → Backward
    GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 1);
    GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 1);
    
    uint8_t leftSpeed = (speed > RIGHT_MOTOR_BOOST) ? (speed - RIGHT_MOTOR_BOOST) : 0;
    
    UART_SendString("[PIVOT RIGHT] L=");
    UART_SendNumber(leftSpeed);
    UART_SendString(" R=");
    UART_SendNumber(speed);
    UART_SendString("\r\n");
    
    Motor_SetBothPWM(leftSpeed, speed);
}

void Motor_Stop(void)
{
    // Set both motors to coast (IN1=0, IN2=0)
    GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 0);
    GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 0);
    
    UART_SendString("[STOP]\r\n");
    
    // Stop PWM on both
    Motor_SetBothPWM(0, 0);
}

uint8_t Motor_GetDefaultSpeed(void)
{
    return defaultSpeed;
}
