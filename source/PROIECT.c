#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "uart.h"

// Uncomment one of these to run a specific test
//#define TEST_LDR_LED
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
void run_test_ldr_led(void);
void run_test_dht11(void);
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
 * @brief Test LDR and LED functionality
 */
void run_test_ldr_led(void)
{
    UART_SendString("\r\n===== TEST LDR + LED =====\r\n");

    Ldr_Init();
    Lights_Init();
    
    UART_SendString("LDR and LED initialized\r\n");
    UART_SendString("Threshold: 1500 (< = LED ON, >= = LED OFF)\r\n\r\n");

    while (1)
    {
        uint16_t val = Ldr_Read();

        UART_SendString("LDR=");
        UART_SendNumber(val);
        
        // Show LED state
        if (val < 3000) {
            UART_SendString(" -> LED ON (dark)\r\n");
        } else {
            UART_SendString(" -> LED OFF (bright)\r\n");
        }

        Lights_Auto(val);

        // Delay ~500ms between readings
        for (volatile uint32_t i = 0; i < 2400000; i++);
    }
}

/**
 * @brief Test DHT11 temperature and humidity sensor
 */
void run_test_dht11(void)
{
    uint16_t temperature, humidity;
    uint32_t read_count = 0;
    uint32_t success_count = 0;
    
    UART_SendString("\r\n===== DHT11 TEMPERATURE & HUMIDITY SENSOR TEST =====\r\n");
    UART_SendString("Initializing DHT11 on PTD4...\r\n");
    
    DHT11_Init();
    
    UART_SendString("DHT11 initialized. Starting measurements...\r\n");
    UART_SendString("Note: DHT11 requires 1 second between readings.\r\n\r\n");
    
    while (1)
    {
        read_count++;
        
        // Attempt to read sensor data
        DHT11_ErrorCode result = DHT11_Read(&temperature, &humidity);
        
        if (result == DHT11_OK)
        {
            success_count++;
            
            // Display the results
            UART_SendString("--- Reading #");
            UART_SendNumber(read_count);
            UART_SendString(" (Success: ");
            UART_SendNumber(success_count);
            UART_SendString("/");
            UART_SendNumber(read_count);
            UART_SendString(") ---\r\n");
            
            UART_SendString("Temperature: ");
            UART_SendNumber(temperature / 100);
            UART_SendString(".");
            if ((temperature % 100) < 10) UART_SendString("0");
            UART_SendNumber(temperature % 100);
            UART_SendString(" C\r\n");
            
            UART_SendString("Humidity:    ");
            UART_SendNumber(humidity / 100);
            UART_SendString(".");
            if ((humidity % 100) < 10) UART_SendString("0");
            UART_SendNumber(humidity % 100);
            UART_SendString(" %\r\n");
            UART_SendString("Status:      VALID\r\n\r\n");
            
            // Check for abnormal conditions
            if (temperature > 3000) { // > 30.00°C
                UART_SendString("WARNING: High temperature detected!\r\n");
            }
            if (humidity > 8000) { // > 80.00%
                UART_SendString("WARNING: High humidity detected!\r\n");
            }
            if (temperature < 1000) { // < 10.00°C
                UART_SendString("WARNING: Low temperature detected!\r\n");
            }
        }
        else
        {
            UART_SendString("--- Reading #");
            UART_SendNumber(read_count);
            UART_SendString(" (FAILED) ---\r\n");
            UART_SendString("Error: ");
            UART_SendString(DHT11_GetErrorString(result));
            UART_SendString("\r\n");
            UART_SendString("Possible causes:\r\n");
            UART_SendString("  - Wiring issue (check VCC, GND, DATA connections)\r\n");
            UART_SendString("  - Missing pull-up resistor on DATA line (10kOhm recommended)\r\n");
            UART_SendString("  - Sensor not powered properly\r\n");
            UART_SendString("  - Timing issues (check clock configuration)\r\n\r\n");
        }
        
        // DHT11 requires minimum 2 seconds between readings
        // Delay approximately 2.5 seconds
        for (volatile uint32_t i = 0; i < 50; i++) {
            for (volatile uint32_t j = 0; j < 1000000; j++);
        }
    }
}


// PIT turn timer for motor test
#define TEST_TURN_DURATION_MS  400U   // 0.4 seconds for ~90 degree turn
static volatile bool g_testTurnComplete = false;

/**
 * @brief Initialize PIT channel 1 for motor test turn timing
 */
static void TestTurnTimer_Init(void)
{
    // Enable PIT clock (may already be enabled)
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    
    // Enable PIT module
    PIT->MCR = 0;
    
    // Disable PIT1 initially
    PIT->CHANNEL[1].TCTRL = 0;
    
    // Enable PIT interrupt in NVIC
    NVIC_EnableIRQ(PIT_IRQn);
    NVIC_SetPriority(PIT_IRQn, 2);
}

/**
 * @brief Start one-shot turn timer on PIT channel 1
 */
static void TestTurnTimer_Start(void)
{
    uint32_t busClk = CLOCK_GetBusClkFreq();
    uint32_t loadVal = (busClk / 1000) * TEST_TURN_DURATION_MS - 1;
    
    g_testTurnComplete = false;
    
    // Configure PIT1
    PIT->CHANNEL[1].LDVAL = loadVal;
    PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;  // Clear any pending flag
    PIT->CHANNEL[1].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;  // Enable timer + interrupt
}

/**
 * @brief PIT1 interrupt handler for motor test turns
 * Note: This will be called from PIT_IRQHandler - check for channel 1
 */
void TestTurnTimer_IRQHandler(void)
{
    if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
        PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;  // Clear flag
        PIT->CHANNEL[1].TCTRL = 0;  // Disable timer (one-shot)
        g_testTurnComplete = true;
    }
}

/**
 * @brief Wait for turn to complete (non-blocking poll)
 */
static void WaitForTurnComplete(void)
{
    while (!g_testTurnComplete) {
        // Just wait - interrupt will set the flag
    }
    Motor_Stop();
    UART_SendString("Turn complete!\r\n");
}

/**
 * @brief Test motor functionality
 */
void run_test_motors(void)
{
    UART_SendString("Motor Test Starting...\r\n");
    
    Motor_Init();
    TestTurnTimer_Init();
    
    while (1) {
        UART_SendString("Forward... (10s)\r\n");
        Motor_Forward(100); 
        for (volatile uint32_t i = 0; i < 24000000; i++);  // ~10 seconds
        
        UART_SendString("Stop... (5s)\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 12000000; i++);  // ~5 seconds
        
        UART_SendString("Backward... (10s)\r\n");
        Motor_Backward(100);
        for (volatile uint32_t i = 0; i < 24000000; i++);
        
        UART_SendString("Stop... (5s)\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 12000000; i++);
        
        // === Turn Left using PIT timer ===
        UART_SendString("Turn Left... (PIT timer)\r\n");
        Motor_TurnLeft(100);
        TestTurnTimer_Start();
        WaitForTurnComplete();
        
        UART_SendString("Stop... (2s)\r\n");
        for (volatile uint32_t i = 0; i < 4800000; i++);  // ~2 seconds pause
        
        // === Turn Right using PIT timer ===
        UART_SendString("Turn Right... (PIT timer)\r\n");
        Motor_TurnRight(100);
        TestTurnTimer_Start();
        WaitForTurnComplete();
        
        UART_SendString("Stop - Cycle Complete\r\n\r\n");
        Motor_Stop();
        for (volatile uint32_t i = 0; i < 24000000; i++);
    }
}

/**
 * @brief Test ultrasonic sensors (FRONT and REAR)
 */
void run_test_ultrasonic(void)
{
    UART_SendString("Dual Ultrasonic Sensor Test\r\n");
    UART_SendString("FRONT: PTC8/PTC9, REAR: PTC8/PTA12\r\n\r\n");
    
    Ultrasonic_Init();
    
    while (1) {
        // Read FRONT sensor
        uint32_t frontDistance = Ultrasonic_GetDistanceCm();
        
        UART_SendString("FRONT: ");
        UART_SendNumber(frontDistance);
        UART_SendString(" cm");
        if (frontDistance < OBSTACLE_THRESHOLD_CM) {
            UART_SendString(" OBSTACLE!");
        }
        
        UART_SendString("  |  ");
        
        // Small delay between sensor readings to avoid interference
        for (volatile uint32_t i = 0; i < 100000; i++);
        
        // Read REAR sensor
        uint32_t rearDistance = Ultrasonic_GetRearDistanceCm();
        
        UART_SendString("REAR: ");
        UART_SendNumber(rearDistance);
        UART_SendString(" cm");
        if (rearDistance < OBSTACLE_THRESHOLD_CM) {
            UART_SendString(" OBSTACLE!");
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
    
    // Distance FRONT
    uint32_t frontDistance = Ultrasonic_GetDistanceCm();
    Bluetooth_SendString("FRONT: ");
    Bluetooth_SendNumber(frontDistance);
    Bluetooth_SendString(" cm\r\n");
    
    // Distance REAR
    uint32_t rearDistance = Ultrasonic_GetRearDistanceCm();
    Bluetooth_SendString("REAR: ");
    Bluetooth_SendNumber(rearDistance);
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
    DHT11_Init(); 
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
        // 2. Obstacle Detection (FRONT when FORWARD, REAR when BACKWARD)
        // =========================================
        if (FSM_GetState() == STATE_FORWARD) {
            uint32_t distance = Ultrasonic_GetDistanceCm();
            
            if (distance < OBSTACLE_THRESHOLD_CM && distance > 0 && distance < 500) {
                // Front obstacle detected! Override any pending event
                event = EVENT_OBSTACLE;
                FSM_SendObstacleAlert(distance);
            }
        }
        else if (FSM_GetState() == STATE_BACKWARD) {
            uint32_t rearDistance = Ultrasonic_GetRearDistanceCm();
            
            if (rearDistance < OBSTACLE_THRESHOLD_CM && rearDistance > 0 && rearDistance < 500) {
                // Rear obstacle detected! Override any pending event
                event = EVENT_OBSTACLE;
                Bluetooth_SendString("!! REAR OBSTACLE at ");
                Bluetooth_SendNumber(rearDistance);
                Bluetooth_SendString(" cm - STOPPED !!\r\n");
            }
        }
        
        // =========================================
        // 3. Process event through FSM
        // =========================================
        if (event != EVENT_NONE) {
            FSM_ProcessEvent(event);
        }
        
        // =========================================
        // 4. Update FSM (check for turn completion)
        // =========================================
        FSM_Update();
        
        // =========================================
        // 5. Auto Lights (LDR polling - separate from FSM)
        // =========================================
        if (autoLightsMode) {
            uint16_t ldr_value = Ldr_Read();
            Lights_Auto(ldr_value);
        }
    }
}
