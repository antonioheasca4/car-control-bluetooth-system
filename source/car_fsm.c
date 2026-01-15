#include "car_fsm.h"
#include "motor.h"
#include "bluetooth.h"
#include "uart.h"

/**
 * Car Control Finite State Machine Implementation
 * 
 * State transitions:
 * - IDLE -> FORWARD/BACKWARD/LEFT/RIGHT via Bluetooth commands
 * - FORWARD -> IDLE on obstacle detection or STOP command
 * - Any moving state -> IDLE on STOP command
 * - Moving states can transition directly between each other
 */

// FSM State
static CarState_t g_currentState = STATE_IDLE;
static uint8_t g_currentSpeed = 70;  // Default speed 70%

// State name strings for debugging
static const char* stateNames[] = {
    "IDLE",
    "FORWARD",
    "BACKWARD",
    "LEFT",
    "RIGHT"
};

/**
 * @brief Execute action for entering a state
 */
static void FSM_EnterState(CarState_t newState)
{
    g_currentState = newState;
    
    switch (newState) {
        case STATE_IDLE:
            Motor_Stop();
            Bluetooth_SendString(">> State: IDLE\r\n");
            break;
            
        case STATE_FORWARD:
            Motor_Forward(g_currentSpeed);
            Bluetooth_SendString(">> State: FORWARD\r\n");
            break;
            
        case STATE_BACKWARD:
            Motor_Backward(g_currentSpeed);
            Bluetooth_SendString(">> State: BACKWARD\r\n");
            break;
            
        case STATE_LEFT:
            Motor_TurnLeft(g_currentSpeed);
            Bluetooth_SendString(">> State: LEFT\r\n");
            break;
            
        case STATE_RIGHT:
            Motor_TurnRight(g_currentSpeed);
            Bluetooth_SendString(">> State: RIGHT\r\n");
            break;
    }
}

/**
 * @brief Handle events when in IDLE state
 */
static void FSM_HandleIdleState(CarEvent_t event)
{
    switch (event) {
        case EVENT_CMD_FORWARD:
            FSM_EnterState(STATE_FORWARD);
            break;
        case EVENT_CMD_BACKWARD:
            FSM_EnterState(STATE_BACKWARD);
            break;
        case EVENT_CMD_LEFT:
            FSM_EnterState(STATE_LEFT);
            break;
        case EVENT_CMD_RIGHT:
            FSM_EnterState(STATE_RIGHT);
            break;
        default:
            // Ignore other events in IDLE
            break;
    }
}

/**
 * @brief Handle events when in FORWARD state
 * Special handling: obstacle detection causes immediate stop + alert
 */
static void FSM_HandleForwardState(CarEvent_t event)
{
    switch (event) {
        case EVENT_OBSTACLE:
            // Obstacle detected! Stop immediately and alert
            FSM_EnterState(STATE_IDLE);
            // Alert is sent by caller with distance info
            break;
        case EVENT_CMD_STOP:
            FSM_EnterState(STATE_IDLE);
            break;
        case EVENT_CMD_BACKWARD:
            FSM_EnterState(STATE_BACKWARD);
            break;
        case EVENT_CMD_LEFT:
            FSM_EnterState(STATE_LEFT);
            break;
        case EVENT_CMD_RIGHT:
            FSM_EnterState(STATE_RIGHT);
            break;
        case EVENT_CMD_FORWARD:
            // Already forward, maybe update speed?
            Motor_Forward(g_currentSpeed);
            break;
        default:
            break;
    }
}

/**
 * @brief Handle events when in BACKWARD state
 * Note: No obstacle detection when going backward
 */
static void FSM_HandleBackwardState(CarEvent_t event)
{
    switch (event) {
        case EVENT_CMD_STOP:
            FSM_EnterState(STATE_IDLE);
            break;
        case EVENT_CMD_FORWARD:
            FSM_EnterState(STATE_FORWARD);
            break;
        case EVENT_CMD_LEFT:
            FSM_EnterState(STATE_LEFT);
            break;
        case EVENT_CMD_RIGHT:
            FSM_EnterState(STATE_RIGHT);
            break;
        case EVENT_CMD_BACKWARD:
            // Already backward, maybe update speed?
            Motor_Backward(g_currentSpeed);
            break;
        default:
            break;
    }
}

/**
 * @brief Handle events when in LEFT state
 */
static void FSM_HandleLeftState(CarEvent_t event)
{
    switch (event) {
        case EVENT_CMD_STOP:
            FSM_EnterState(STATE_IDLE);
            break;
        case EVENT_CMD_FORWARD:
            FSM_EnterState(STATE_FORWARD);
            break;
        case EVENT_CMD_BACKWARD:
            FSM_EnterState(STATE_BACKWARD);
            break;
        case EVENT_CMD_RIGHT:
            FSM_EnterState(STATE_RIGHT);
            break;
        case EVENT_CMD_LEFT:
            // Already turning left
            Motor_TurnLeft(g_currentSpeed);
            break;
        default:
            break;
    }
}

/**
 * @brief Handle events when in RIGHT state
 */
static void FSM_HandleRightState(CarEvent_t event)
{
    switch (event) {
        case EVENT_CMD_STOP:
            FSM_EnterState(STATE_IDLE);
            break;
        case EVENT_CMD_FORWARD:
            FSM_EnterState(STATE_FORWARD);
            break;
        case EVENT_CMD_BACKWARD:
            FSM_EnterState(STATE_BACKWARD);
            break;
        case EVENT_CMD_LEFT:
            FSM_EnterState(STATE_LEFT);
            break;
        case EVENT_CMD_RIGHT:
            // Already turning right
            Motor_TurnRight(g_currentSpeed);
            break;
        default:
            break;
    }
}

// ============================================
// Public API Implementation
// ============================================

void FSM_Init(void)
{
    g_currentState = STATE_IDLE;
    g_currentSpeed = 70;
    Motor_Stop();
    UART_SendString("  FSM initialized (IDLE)\r\n");
}

void FSM_ProcessEvent(CarEvent_t event)
{
    if (event == EVENT_NONE) {
        return;
    }
    
    switch (g_currentState) {
        case STATE_IDLE:
            FSM_HandleIdleState(event);
            break;
        case STATE_FORWARD:
            FSM_HandleForwardState(event);
            break;
        case STATE_BACKWARD:
            FSM_HandleBackwardState(event);
            break;
        case STATE_LEFT:
            FSM_HandleLeftState(event);
            break;
        case STATE_RIGHT:
            FSM_HandleRightState(event);
            break;
    }
}

CarState_t FSM_GetState(void)
{
    return g_currentState;
}

const char* FSM_GetStateName(CarState_t state)
{
    if (state <= STATE_RIGHT) {
        return stateNames[state];
    }
    return "UNKNOWN";
}

void FSM_SetSpeed(uint8_t speed)
{
    if (speed > 100) speed = 100;
    g_currentSpeed = speed;
    
    // Update motor speed if currently moving
    switch (g_currentState) {
        case STATE_FORWARD:
            Motor_Forward(g_currentSpeed);
            break;
        case STATE_BACKWARD:
            Motor_Backward(g_currentSpeed);
            break;
        case STATE_LEFT:
            Motor_TurnLeft(g_currentSpeed);
            break;
        case STATE_RIGHT:
            Motor_TurnRight(g_currentSpeed);
            break;
        default:
            break;
    }
}

uint8_t FSM_GetSpeed(void)
{
    return g_currentSpeed;
}

bool FSM_IsMoving(void)
{
    return (g_currentState != STATE_IDLE);
}

void FSM_StopCar(void)
{
    FSM_EnterState(STATE_IDLE);
}

void FSM_SendObstacleAlert(uint32_t distanceCm)
{
    Bluetooth_SendString("!! OBSTACLE at ");
    Bluetooth_SendNumber(distanceCm);
    Bluetooth_SendString(" cm - STOPPED !!\r\n");
}
