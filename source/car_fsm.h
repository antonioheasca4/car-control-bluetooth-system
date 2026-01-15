#ifndef CAR_FSM_H
#define CAR_FSM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Car Control Finite State Machine
 * 
 * Implements a state machine for car movement control with:
 * - 5 states: IDLE, FORWARD, BACKWARD, LEFT, RIGHT
 * - Events from Bluetooth commands and obstacle sensor
 * - Automatic obstacle detection and alerting when moving forward
 */

/**
 * @brief FSM States
 */
typedef enum {
    STATE_IDLE = 0,     // Car is stopped, waiting for commands
    STATE_FORWARD,      // Car is moving forward (obstacle detection active)
    STATE_BACKWARD,     // Car is moving backward
    STATE_LEFT,         // Car is turning left
    STATE_RIGHT         // Car is turning right
} CarState_t;

/**
 * @brief FSM Events
 */
typedef enum {
    EVENT_NONE = 0,         // No event
    EVENT_CMD_FORWARD,      // Bluetooth command: Forward (F/W)
    EVENT_CMD_BACKWARD,     // Bluetooth command: Backward (B/X)
    EVENT_CMD_LEFT,         // Bluetooth command: Left (L/A)
    EVENT_CMD_RIGHT,        // Bluetooth command: Right (R/D)
    EVENT_CMD_STOP,         // Bluetooth command: Stop (S/space)
    EVENT_OBSTACLE,         // Obstacle detected (<20cm)
    EVENT_OBSTACLE_CLEAR    // Obstacle cleared
} CarEvent_t;

/**
 * @brief Initialize the FSM
 * Sets initial state to IDLE and default speed
 */
void FSM_Init(void);

/**
 * @brief Process an event and perform state transition
 * @param event The event to process
 */
void FSM_ProcessEvent(CarEvent_t event);

/**
 * @brief Get current FSM state
 * @return Current state
 */
CarState_t FSM_GetState(void);

/**
 * @brief Get state name as string (for debugging)
 * @param state The state to convert
 * @return String representation of state
 */
const char* FSM_GetStateName(CarState_t state);

/**
 * @brief Set motor speed for FSM actions
 * @param speed Speed percentage 0-100
 */
void FSM_SetSpeed(uint8_t speed);

/**
 * @brief Get current speed setting
 * @return Speed percentage 0-100
 */
uint8_t FSM_GetSpeed(void);

/**
 * @brief Check if car is currently moving
 * @return true if in a moving state (not IDLE)
 */
bool FSM_IsMoving(void);

/**
 * @brief Stop the car and transition to IDLE
 * Called internally when obstacle detected or STOP command received
 */
void FSM_StopCar(void);

/**
 * @brief Send obstacle alert via Bluetooth
 * @param distanceCm Distance to obstacle in cm
 */
void FSM_SendObstacleAlert(uint32_t distanceCm);

#endif // CAR_FSM_H
