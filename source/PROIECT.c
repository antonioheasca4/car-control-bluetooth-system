#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "uart.h"

// Tests
#include "tests/test_ldr_led.h"
#include "tests/test_dht11.h"

// Uncomment one of these to run a specific test
// #define TEST_LDR_LED
// #define TEST_DHT11
// #define TEST_MOTORS
// #define TEST_ULTRASONIC

// Drivers
#include "ldr.h"
#include "lights.h"
#include "dht11.h"
#include "motor.h"
#include "ultrasonic.h"
#include "bluetooth.h"
#include "car_fsm.h"

// Configuration
#define OBSTACLE_THRESHOLD_CM   20      // Stop if obstacle closer than 20cm
#define AUTO_LIGHTS_ENABLED     1       // 1 = auto lights on by default

// Global state (separate from FSM - for lights only)
static uint8_t autoLightsMode = AUTO_LIGHTS_ENABLED;

// Function prototypes
void run_main_application(void);
void run_test_motors(void);
void run_test_ultrasonic(void);
CarEvent_t ConvertBluetoothToEvent(BluetoothCommand cmd);
void ProcessNonMovementCommand(BluetoothCommand cmd, uint8_t speed);
void SendSensorInfo(void);

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    
    // Initialize debug console (UART0 for printf and Bluetooth)
    BOARD_InitDebugConsole();

#ifdef TEST_LDR_LED
    run_test_ldr_led();
#elif defined(TEST_DHT11)
    run_test_dht11();
#elif defined(TEST_MOTORS)
    run_test_motors();
#elif defined(TEST_ULTRASONIC)
    run_test_ultrasonic();
#else
    run_main_application();
#endif

    return 0;
}

/**
 * @brief Test motor functionality
 */
void run_test_motors(void)
{
    UART_SendString("Motor Test Starting...\r\n");
    
    Motor_Init();
    
    while (1) {
        UART_SendString("Forward... (10s)\r\n");
        Motor_Forward(70);
        for (volatile uint32_t i = 0; i < 24000000; i++);  // ~10 seconds
        
        UART_SendString("Stop... (5s)\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 12000000; i++);  // ~5 seconds
        
        UART_SendString("Backward... (10s)\r\n");
        Motor_Backward(70);
        for (volatile uint32_t i = 0; i < 24000000; i++);
        
        UART_SendString("Stop... (5s)\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 12000000; i++);
        
        UART_SendString("Turn Left... (10s)\r\n");
        Motor_TurnLeft(70);
        for (volatile uint32_t i = 0; i < 24000000; i++);
        
        UART_SendString("Turn Right... (10s)\r\n");
        Motor_TurnRight(70);
        for (volatile uint32_t i = 0; i < 24000000; i++);
        
        UART_SendString("Stop - Cycle Complete\r\n\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 24000000; i++);
    }
}

/**
 * @brief Test ultrasonic sensor
 */
void run_test_ultrasonic(void)
{
    UART_SendString("Ultrasonic Sensor Test\r\n");
    
    Ultrasonic_Init();
    
    while (1) {
        uint32_t distance = Ultrasonic_GetDistanceCm();
        
        UART_SendString("Distance: ");
        UART_SendNumber(distance);
        UART_SendString(" cm");
        
        if (distance < OBSTACLE_THRESHOLD_CM) {
            UART_SendString(" - OBSTACLE!");
        }
        UART_SendString("\r\n");
        
        // Wait ~500ms between readings
        for (volatile uint32_t i = 0; i < 1200000; i++);
    }
}

/**
 * @brief Convert Bluetooth command to FSM event
 * @param cmd Bluetooth command
 * @return Corresponding FSM event (or EVENT_NONE for non-movement commands)
 */
CarEvent_t ConvertBluetoothToEvent(BluetoothCommand cmd)
{
    switch (cmd) {
        case CMD_FORWARD:   return EVENT_CMD_FORWARD;
        case CMD_BACKWARD:  return EVENT_CMD_BACKWARD;
        case CMD_LEFT:      return EVENT_CMD_LEFT;
        case CMD_RIGHT:     return EVENT_CMD_RIGHT;
        case CMD_STOP:      return EVENT_CMD_STOP;
        default:            return EVENT_NONE;
    }
}

/**
 * @brief Process non-movement Bluetooth commands (lights, sensors, speed)
 * These are handled separately from the FSM
 */
void ProcessNonMovementCommand(BluetoothCommand cmd, uint8_t speed)
{
    switch (cmd) {
        case CMD_LIGHTS_ON:
            Bluetooth_SendString(">> Lights ON\r\n");
            autoLightsMode = 0;
            Lights_On();
            break;
            
        case CMD_LIGHTS_OFF:
            Bluetooth_SendString(">> Lights OFF\r\n");
            autoLightsMode = 0;
            Lights_Off();
            break;
            
        case CMD_LIGHTS_AUTO:
            autoLightsMode = !autoLightsMode;
            Bluetooth_SendString(autoLightsMode ? ">> Auto-lights ON\r\n" : ">> Auto-lights OFF\r\n");
            break;
            
        case CMD_GET_TEMP:
        case CMD_GET_HUMIDITY:
        case CMD_GET_DISTANCE:
        case CMD_GET_INFO:
            SendSensorInfo();
            break;
            
        case CMD_SET_SPEED:
            FSM_SetSpeed(speed);
            Bluetooth_SendString("Speed: ");
            Bluetooth_SendNumber(speed);
            Bluetooth_SendString("%\r\n");
            break;
            
        case CMD_UNKNOWN:
            Bluetooth_SendString("? Unknown command\r\n");
            break;
            
        default:
            break;
    }
}

/**
 * @brief Send all sensor information via Bluetooth
 */
void SendSensorInfo(void)
{
    uint16_t temperature, humidity;
    
    Bluetooth_SendString("=== Sensor Info ===\r\n");
    
    // Current FSM state
    Bluetooth_SendString("State: ");
    Bluetooth_SendString(FSM_GetStateName(FSM_GetState()));
    Bluetooth_SendString("\r\n");
    
    // Distance
    uint32_t distance = Ultrasonic_GetDistanceCm();
    Bluetooth_SendString("Distance: ");
    Bluetooth_SendNumber(distance);
    Bluetooth_SendString(" cm\r\n");
    
    // LDR
    uint16_t ldr = Ldr_Read();
    Bluetooth_SendString("Light: ");
    Bluetooth_SendNumber(ldr);
    Bluetooth_SendString(" (ADC)\r\n");
    
    // DHT11 - read directly (user manually triggers via 'I' command)
    DHT11_ErrorCode result = DHT11_Read(&temperature, &humidity);
    
    if (result == DHT11_OK) {
        Bluetooth_SendString("Temp: ");
        Bluetooth_SendNumber(temperature / 100);
        Bluetooth_SendString(".");
        Bluetooth_SendNumber(temperature % 100);
        Bluetooth_SendString(" C\r\n");
        
        Bluetooth_SendString("Humidity: ");
        Bluetooth_SendNumber(humidity / 100);
        Bluetooth_SendString("%\r\n");
    } else {
        Bluetooth_SendString("DHT11: Error - ");
        Bluetooth_SendString(DHT11_GetErrorString(result));
        Bluetooth_SendString("\r\n");
    }
    
    Bluetooth_SendString("==================\r\n");
}

/**
 * @brief Main application loop using FSM architecture
 * 
 * FSM handles: IDLE, FORWARD, BACKWARD, LEFT, RIGHT states
 * Separate polling: LDR-based auto-lights (independent of car movement)
 */
void run_main_application(void)
{
    UART_SendString("\r\n");
    UART_SendString("================================\r\n");
    UART_SendString("  Bluetooth Car Control System  \r\n");
    UART_SendString("       (FSM Architecture)       \r\n");
    UART_SendString("================================\r\n");
    UART_SendString("Commands:\r\n");
    UART_SendString("  F/W=Forward B/X=Back L/A=Left R/D=Right S=Stop\r\n");
    UART_SendString("  O=LightsON P=LightsOFF M=AutoMode\r\n");
    UART_SendString("  T=Temp H=Humidity U=Distance I=Info\r\n");
    UART_SendString("  1-9=Set Speed (10%-90%)\r\n");
    UART_SendString("================================\r\n\r\n");

    // Initialize all modules
    Ldr_Init();
    Lights_Init();
    DHT11_Init();  // Initialize DHT11 (needs 1 sec stabilization)
    Motor_Init();
    Ultrasonic_Init();
    Bluetooth_Init();
    
    // Initialize FSM (starts in IDLE state)
    FSM_Init();
    
    UART_SendString("System Ready!\r\n");
    
    while (1)
    {
        CarEvent_t event = EVENT_NONE;
        
        // =========================================
        // 1. Read and convert Bluetooth command to FSM event
        // =========================================
        BluetoothCommand cmd = Bluetooth_GetCommand();
        if (cmd != CMD_NONE) {
            // Try to convert to FSM event (movement commands)
            event = ConvertBluetoothToEvent(cmd);
            
            // If not a movement command, process separately
            if (event == EVENT_NONE) {
                ProcessNonMovementCommand(cmd, Bluetooth_GetSpeed());
            }
        }
        
        // =========================================
        // 2. Obstacle Detection (only when moving FORWARD)
        // =========================================
        if (FSM_GetState() == STATE_FORWARD) {
            uint32_t distance = Ultrasonic_GetDistanceCm();
            
            if (distance < OBSTACLE_THRESHOLD_CM && distance > 0 && distance < 500) {
                // Obstacle detected! Override any pending event
                event = EVENT_OBSTACLE;
                FSM_SendObstacleAlert(distance);
            }
        }
        
        // =========================================
        // 3. Process event through FSM
        // =========================================
        if (event != EVENT_NONE) {
            FSM_ProcessEvent(event);
        }
        
        // =========================================
        // 4. Auto Lights (LDR polling - separate from FSM)
        // =========================================
        if (autoLightsMode) {
            uint16_t ldr_value = Ldr_Read();
            Lights_Auto(ldr_value);
        }
        
        // Small delay (~50ms per iteration)
        for (volatile uint32_t i = 0; i < 240000; i++);
    }
}
