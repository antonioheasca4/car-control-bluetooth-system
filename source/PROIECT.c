#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

// Tests
#include "tests/test_ldr_led.h"

#define TEST_LDR_LED



void run_main_application(void);

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    
    // Initialize debug console (UART0 for printf)
    BOARD_InitDebugConsole();

#ifdef TEST_LDR_LED
    run_test_ldr_led();
#else
    run_main_application();
#endif

    return 0;
}


void run_main_application(void)
{
    PRINTF("Car control system start\r\n");

//    Bluetooth_Init();
//    Motors_Init();
//    Servo_Init();
//    Ultrasonic_Init();
//    Ldr_Init();
//    EnvSensor_Init();
//    Lights_Init();

    while (1)
    {
//        Bluetooth_HandleCommands();
//        Sensors_ReadAll();
//        Control_UpdateLights();
//        Control_UpdateMotors();
//        Status_SendToPhone();
    }
}


