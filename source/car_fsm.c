#include "car_fsm.h"
#include "motor.h"
#include "bluetooth.h"
#include "uart.h"
#include "MKL25Z4.h"
#include "fsl_clock.h"

/**
 * Car Control Finite State Machine Implementation
 * 
 * State transitions:
 * - IDLE -> FORWARD/BACKWARD/LEFT/RIGHT via Bluetooth commands
 * - FORWARD -> IDLE on obstacle detection or STOP command
 * - Any moving state -> IDLE on STOP command
 * - LEFT/RIGHT: 90-degree pivot turn (non-blocking via PIT timer)
 * - Moving states can transition directly between each other
 */

// Turn duration in milliseconds (adjust for 90-degree turn)
#define TURN_DURATION_MS  400U

// FSM State
static CarState_t g_currentState = STATE_IDLE;
static uint8_t g_currentSpeed = 0;  // Will be set from Motor_GetDefaultSpeed()

// Turn timer flag - set by PIT interrupt when turn is complete
static volatile bool g_turnComplete = false;
static volatile bool g_turnActive = false;

// State name strings for debugging
static const char* stateNames[] = {
    "IDLE",
    "FORWARD",
    "BACKWARD",
    "LEFT",
    "RIGHT"
};

/**
 * @brief Initialize PIT channel 0 for turn timing
 */
static void TurnTimer_Init(void)
{
    // Enable PIT clock
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    
    // Enable PIT module
    PIT->MCR = 0;  // Enable PIT, do not freeze in debug
    
    // Disable PIT0 initially
    PIT->CHANNEL[0].TCTRL = 0;
    
    // Enable PIT0 interrupt in NVIC
    NVIC_EnableIRQ(PIT_IRQn);
    NVIC_SetPriority(PIT_IRQn, 2);
}

/**
 * @brief Start one-shot turn timer
 */
static void TurnTimer_Start(void)
{
    // Calculate LDVAL for desired duration
    // PIT clock = Bus clock = 24MHz (CLOCK_GetBusClkFreq())
    uint32_t busClk = CLOCK_GetBusClkFreq();
    uint32_t loadVal = (busClk / 1000) * TURN_DURATION_MS - 1;
    
    g_turnComplete = false;
    g_turnActive = true;
    
    // Configure PIT0
    PIT->CHANNEL[0].LDVAL = loadVal;
    PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;  // Clear any pending flag
    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;  // Enable timer + interrupt
}

/**
 * @brief Stop turn timer
 */
static void TurnTimer_Stop(void)
{
    PIT->CHANNEL[0].TCTRL = 0;  // Disable timer
    g_turnActive = false;
}

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
            Bluetooth_SendString(">> Pivot LEFT 90deg...\r\n");
            Motor_TurnLeft(g_currentSpeed);
            TurnTimer_Start();  // Non-blocking timer
            break;
            
        case STATE_RIGHT:
            Bluetooth_SendString(">> Pivot RIGHT 90deg...\r\n");
            Motor_TurnRight(g_currentSpeed);
            TurnTimer_Start();  // Non-blocking timer
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
 * Obstacle detection added for rear sensor
 */
static void FSM_HandleBackwardState(CarEvent_t event)
{
    switch (event) {
        case EVENT_OBSTACLE:
            // Rear obstacle detected! Stop immediately
            FSM_EnterState(STATE_IDLE);
            break;
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
    g_currentSpeed = Motor_GetDefaultSpeed();  // Get default from motor.c
    g_turnComplete = false;
    g_turnActive = false;
    
    TurnTimer_Init();  // Initialize PIT for turn timing
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

/**
 * @brief Update FSM - call this from main loop
 * Checks if turn is complete and transitions to IDLE
 */
void FSM_Update(void)
{
    if (g_turnComplete && (g_currentState == STATE_LEFT || g_currentState == STATE_RIGHT)) {
        Motor_Stop();
        g_currentState = STATE_IDLE;
        g_turnComplete = false;
        Bluetooth_SendString(">> Turn complete -> IDLE\r\n");
    }
}

/**
 * @brief PIT Interrupt Handler
 * Called when turn timer expires
 */
void PIT_IRQHandler(void)
{
    // Check if PIT0 caused interrupt (FSM turn timer)
    if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) {
        // Clear interrupt flag
        PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
        
        // Disable timer (one-shot)
        PIT->CHANNEL[0].TCTRL = 0;
        
        // Set completion flag
        g_turnComplete = true;
        g_turnActive = false;
    }
    
    // Check if PIT1 caused interrupt (Motor test turn timer)
    if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
        // Forward to test handler defined in PROIECT.c
        extern void TestTurnTimer_IRQHandler(void);
        TestTurnTimer_IRQHandler();
    }
}
