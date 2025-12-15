#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * HC-SR04 Ultrasonic Distance Sensor Module
 * 
 * Connections (UPDATED for available header pins):
 *   - TRIG: PTC8 (J1) - GPIO Output
 *   - ECHO: PTC9 (J1) - GPIO Input - USE VOLTAGE DIVIDER!
 *   - VCC:  5V (external)
 *   - GND:  Common ground with FRDM
 * 
 * WARNING: ECHO pin outputs 5V! Use voltage divider:
 *   ECHO --[1kΩ]--+-- PTC9
 *                 |
 *              [2kΩ]
 *                 |
 *                GND
 */

// Measurement range
#define ULTRASONIC_MIN_DISTANCE_CM  2U      // Minimum reliable distance
#define ULTRASONIC_MAX_DISTANCE_CM  400U    // Maximum reliable distance
#define ULTRASONIC_TIMEOUT_CM       500U    // Return this on timeout/error

/**
 * @brief Initialize ultrasonic sensor GPIO pins
 */
void Ultrasonic_Init(void);

/**
 * @brief Get distance measurement in centimeters
 * @return Distance in cm (2-400), or ULTRASONIC_TIMEOUT_CM on error
 */
uint32_t Ultrasonic_GetDistanceCm(void);

/**
 * @brief Check if obstacle is within threshold distance
 * @param thresholdCm Threshold distance in cm
 * @return true if obstacle detected within threshold
 */
bool Ultrasonic_ObstacleDetected(uint32_t thresholdCm);

/**
 * @brief Get raw echo pulse duration in microseconds
 * @return Pulse duration in µs, or 0 on timeout
 */
uint32_t Ultrasonic_GetPulseUs(void);

#endif // ULTRASONIC_H
