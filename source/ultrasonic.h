#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Dual HC-SR04 Ultrasonic Distance Sensors
 * 
 * Connections:
 *   FRONT Sensor:
 *   - TRIG: PTC8 (shared) - GPIO Output
 *   - ECHO: PTC9 - GPIO Input (needs voltage divider 5V->3.3V)
 * 
 *   REAR Sensor:
 *   - TRIG: PTC8 (shared) - GPIO Output  
 *   - ECHO: PTA12 - GPIO Input (needs voltage divider 5V->3.3V)
 *   
 *   - VCC:  5V (external)
 *   - GND:  Common ground with FRDM
 */

// Sensor selection
typedef enum {
    ULTRASONIC_FRONT = 0,
    ULTRASONIC_REAR  = 1
} UltrasonicSensor_t;

// Measurement range
#define ULTRASONIC_MIN_DISTANCE_CM  2U      // Minimum reliable distance
#define ULTRASONIC_MAX_DISTANCE_CM  400U    // Maximum reliable distance
#define ULTRASONIC_TIMEOUT_CM       500U    // Return this on timeout/error

/**
 * @brief Initialize both ultrasonic sensor GPIO pins
 */
void Ultrasonic_Init(void);

/**
 * @brief Get distance measurement from specific sensor
 * @param sensor Which sensor to read (ULTRASONIC_FRONT or ULTRASONIC_REAR)
 * @return Distance in cm (2-400), or ULTRASONIC_TIMEOUT_CM on error
 */
uint32_t Ultrasonic_GetDistanceCm_Sensor(UltrasonicSensor_t sensor);

/**
 * @brief Get distance measurement from FRONT sensor (backward compatible)
 * @return Distance in cm (2-400), or ULTRASONIC_TIMEOUT_CM on error
 */
uint32_t Ultrasonic_GetDistanceCm(void);

/**
 * @brief Get distance measurement from REAR sensor
 * @return Distance in cm (2-400), or ULTRASONIC_TIMEOUT_CM on error
 */
uint32_t Ultrasonic_GetRearDistanceCm(void);

/**
 * @brief Check if obstacle is within threshold distance
 * @param thresholdCm Threshold distance in cm
 * @return true if obstacle detected within threshold
 */
bool Ultrasonic_ObstacleDetected(uint32_t thresholdCm);

/**
 * @brief Check if obstacle is behind (rear sensor)
 * @param thresholdCm Threshold distance in cm
 * @return true if obstacle detected within threshold
 */
bool Ultrasonic_RearObstacleDetected(uint32_t thresholdCm);

/**
 * @brief Get raw echo pulse duration in microseconds from specific sensor
 * @param sensor Which sensor to read
 * @return Pulse duration in µs, or 0 on timeout
 */
uint32_t Ultrasonic_GetPulseUs(UltrasonicSensor_t sensor);

#endif // ULTRASONIC_H
