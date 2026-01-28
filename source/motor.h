#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * Motor Control Module for L293D Driver
 * 
 * Motor Left:  IN1=PTB1, IN2=PTB2, EN=PTA4 (TPM0_CH1)
 * Motor Right: IN1=PTB3, IN2=PTC2, EN=PTA5 (TPM0_CH2)
 * 
 * All pins on J1 and J10 headers
 * Differential steering: turning is achieved by varying motor speeds
 */

/**
 * @brief Initialize motor control GPIO and PWM
 */
void Motor_Init(void);

/**
 * @brief Move forward at given speed
 * @param speed Speed percentage 0-100
 */
void Motor_Forward(uint8_t speed);

/**
 * @brief Move backward at given speed
 * @param speed Speed percentage 0-100
 */
void Motor_Backward(uint8_t speed);

/**
 * @brief Turn left (pivot) - right motor forward, left motor backward
 * @param speed Speed percentage 0-100
 */
void Motor_TurnLeft(uint8_t speed);

/**
 * @brief Turn right (pivot) - left motor forward, right motor backward
 * @param speed Speed percentage 0-100
 */
void Motor_TurnRight(uint8_t speed);

/**
 * @brief Stop both motors immediately
 */
void Motor_Stop(void);

/**
 * @brief Get default speed
 * @return Default speed percentage 0-100
 */
uint8_t Motor_GetDefaultSpeed(void);

#endif // MOTOR_H
