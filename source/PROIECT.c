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

// Configuration
#define OBSTACLE_THRESHOLD_CM   20      // Stop if obstacle closer than 20cm
#define DHT11_READ_INTERVAL     100     // Read DHT11 every 100 iterations (~5 seconds)
#define AUTO_LIGHTS_ENABLED     1       // 1 = auto lights on by default

// Global state
static uint8_t autoLightsMode = AUTO_LIGHTS_ENABLED;
static uint8_t isMoving = 0;
static uint8_t lastDirection = 0;  // 0=none, 1=forward, 2=backward
static uint8_t lastSpeed = 70;     // Last used speed
static uint8_t obstaclePaused = 0; // 1 = stopped due to obstacle

// Function prototypes
void run_main_application(void);
void run_test_motors(void);
void run_test_ultrasonic(void);
void ProcessBluetoothCommand(BluetoothCommand cmd, uint8_t speed);
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
        
//    	UART_SendString("Right FORWARD... (10s)\r\n");
//    	Motor_SetRight(MOTOR_FORWARD, 100);
//    	for (volatile uint32_t i = 0; i < 24000000; i++);

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
 * @brief Process received Bluetooth command
 */
void ProcessBluetoothCommand(BluetoothCommand cmd, uint8_t speed)
{
    switch (cmd) {
        case CMD_FORWARD:
            Bluetooth_SendString(">> Forward\r\n");
            Motor_Forward(speed);
            isMoving = 1;
            lastDirection = 1;  // Remember we're going forward
            lastSpeed = speed;
            obstaclePaused = 0;
            break;
            
        case CMD_BACKWARD:
            Bluetooth_SendString(">> Backward\r\n");
            Motor_Backward(speed);
            isMoving = 1;
            lastDirection = 2;  // Remember we're going backward
            lastSpeed = speed;
            obstaclePaused = 0;
            break;
            
        case CMD_LEFT:
            Bluetooth_SendString(">> Left\r\n");
            Motor_TurnLeft(speed);
            isMoving = 1;
            break;
            
        case CMD_RIGHT:
            Bluetooth_SendString(">> Right\r\n");
            Motor_TurnRight(speed);
            isMoving = 1;
            break;
            
        case CMD_STOP:
            Bluetooth_SendString(">> Stop\r\n");
            Motor_Stop();
            isMoving = 0;
            lastDirection = 0;  // Clear direction so it won't auto-restart
            obstaclePaused = 0;
            break;
            
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
            Bluetooth_SendString("Speed: ");
            Bluetooth_SendNumber(speed);
            Bluetooth_SendString("%\r\n");
            break;
            
        case CMD_UNKNOWN:
            Bluetooth_SendString("? Unknown command\r\n");
            break;
            
        case CMD_NONE:
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
    
    // DHT11
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
 * @brief Main application loop
 */
void run_main_application(void)
{
    UART_SendString("\r\n");
    UART_SendString("================================\r\n");
    UART_SendString("  Bluetooth Car Control System  \r\n");
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
    //DHT11_Init();
    Motor_Init();
    Ultrasonic_Init();
    Bluetooth_Init();

    uint32_t counter = 0;
    uint16_t temperature, humidity;
    uint8_t obstacleWarned = 0;
    
    UART_SendString("System Ready!\r\n");
    
    while (1)
    {
        // =========================================
        // 1. Process Bluetooth Commands
        // =========================================
        BluetoothCommand cmd = Bluetooth_GetCommand();
        if (cmd != CMD_NONE) {
            ProcessBluetoothCommand(cmd, Bluetooth_GetSpeed());
        }
        
        // =========================================
        // 2. Ultrasonic Obstacle Detection (continuous)
        // =========================================
        {
            static uint8_t obstacleAlerted = 0;
            uint32_t distance = Ultrasonic_GetDistanceCm();
            
            if (distance < 20 && distance > 0 && distance < 500) {
                if (!obstacleAlerted) {
                    // Stop the car immediately!
                    Motor_Stop();
                    isMoving = 0;
                    obstaclePaused = 1;  // Remember we paused due to obstacle
                    
                    Bluetooth_SendString("!! OBSTACLE at ");
                    Bluetooth_SendNumber(distance);
                    Bluetooth_SendString(" cm - STOPPED !!\r\n");
                    obstacleAlerted = 1;
                }
            } else {
                // Obstacle cleared - auto-restart if we were paused
                if (obstaclePaused && lastDirection > 0) {
                    Bluetooth_SendString(">> Obstacle cleared - RESUMING\r\n");
                    if (lastDirection == 1) {
                        Motor_Forward(lastSpeed);
                    } else if (lastDirection == 2) {
                        Motor_Backward(lastSpeed);
                    }
                    isMoving = 1;
                    obstaclePaused = 0;
                }
                obstacleAlerted = 0;
            }
        }
        
        // =========================================
        // 3. Auto Lights (LDR based)
        // =========================================
        if (autoLightsMode) {
            uint16_t ldr_value = Ldr_Read();
            Lights_Auto(ldr_value);
        }
        
        // =========================================
        // 4. Periodic DHT11 Reading - DISABLED for testing
        // =========================================
        /*
        if (counter % DHT11_READ_INTERVAL == 0 && counter > 0) {
            // Only send if not in active movement
            if (!isMoving) {
                DHT11_ErrorCode result = DHT11_Read(&temperature, &humidity);
                if (result == DHT11_OK) {
                    // Optional: send periodic temp updates
                    // Bluetooth_SendString("Temp: ");
                    // Bluetooth_SendNumber(temperature / 100);
                    // Bluetooth_SendString("C\r\n");
                }
            }
        }
        */
        
        counter++;
        
        // Small delay (~50ms per iteration)
        for (volatile uint32_t i = 0; i < 240000; i++);
    }
}
