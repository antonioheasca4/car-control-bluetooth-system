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

static uint8_t defaultSpeed = 70;   // Default speed 70%

/**
 * @brief Initialize TPM0 for PWM output
 */
static void Motor_InitPWM(void)
{
    UART_SendString("  Motor_InitPWM() start\r\n");
    
    // IMPORTANT: Set TPM clock source FIRST (before enabling clock gate)
    // Clear existing TPMSRC bits and set to MCGFLLCLK (option 1)
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);
    
    // Then enable TPM0 clock gate (SIM->SCGC6 bit 24)
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    
    uint32_t tpmClock = CLOCK_GetFreq(kCLOCK_McgFllClk);
    UART_SendString("  TPM Clock: ");
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

/**
 * @brief Set PWM duty cycle for a motor
 * @param channel TPM channel (kTPM_Chnl_1 for left, kTPM_Chnl_2 for right)
 * @param percent Duty cycle 0-100
 */
static void Motor_SetPWM(tpm_chnl_t channel, uint8_t percent)
{
    if (percent > 100) percent = 100;
    
    // Calculate duty cycle value based on MOD
    uint32_t mod = TPM0->MOD;
    uint32_t duty = (mod * percent) / 100;
    
    // Update channel value directly
    TPM0->CONTROLS[channel].CnV = duty;
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

void Motor_SetLeft(MotorDirection dir, uint8_t speed)
{
    switch (dir) {
        case MOTOR_FORWARD:
            GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 1);
            GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 0);
            break;
        case MOTOR_BACKWARD:
            GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 0);
            GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 1);
            break;
        case MOTOR_STOP:
        default:
            GPIO_WritePinOutput(MOTOR_L_IN1_GPIO, MOTOR_L_IN1_PIN, 0);
            GPIO_WritePinOutput(MOTOR_L_IN2_GPIO, MOTOR_L_IN2_PIN, 0);
            speed = 0;
            break;
    }
    Motor_SetPWM(kTPM_Chnl_1, speed);  // PTA4 = TPM0_CH1
}

void Motor_SetRight(MotorDirection dir, uint8_t speed)
{
    switch (dir) {
        case MOTOR_FORWARD:
            GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 1);
            GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 0);
            break;
        case MOTOR_BACKWARD:
            GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 0);
            GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 1);
            break;
        case MOTOR_STOP:
        default:
            GPIO_WritePinOutput(MOTOR_R_IN1_GPIO, MOTOR_R_IN1_PIN, 0);
            GPIO_WritePinOutput(MOTOR_R_IN2_GPIO, MOTOR_R_IN2_PIN, 0);
            speed = 0;
            break;
    }
    Motor_SetPWM(kTPM_Chnl_2, speed);
}

void Motor_Forward(uint8_t speed)
{
    Motor_SetLeft(MOTOR_FORWARD, speed);
    Motor_SetRight(MOTOR_FORWARD, speed);
}

void Motor_Backward(uint8_t speed)
{
    Motor_SetLeft(MOTOR_BACKWARD, speed);
    Motor_SetRight(MOTOR_BACKWARD, speed);
}

void Motor_TurnLeft(uint8_t speed)
{
    // Pivot turn: right motor forward, left motor backward (or stopped)
    Motor_SetLeft(MOTOR_BACKWARD, speed / 2);
    Motor_SetRight(MOTOR_FORWARD, speed);
}

void Motor_TurnRight(uint8_t speed)
{
    // Pivot turn: left motor forward, right motor backward (or stopped)
    Motor_SetLeft(MOTOR_FORWARD, speed);
    Motor_SetRight(MOTOR_BACKWARD, speed / 2);
}

void Motor_Stop(void)
{
    Motor_SetLeft(MOTOR_STOP, 0);
    Motor_SetRight(MOTOR_STOP, 0);
}

void Motor_SetDefaultSpeed(uint8_t speed)
{
    if (speed > 100) speed = 100;
    defaultSpeed = speed;
}
