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

// #define TEST_LDR_LED
// #define TEST_DHT11

// Drivers
#include "ldr.h"
#include "lights.h"
#include "dht11.h"


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
#elif defined(TEST_DHT11)
    run_test_dht11();
#else
    run_main_application();
#endif

    return 0;
}


void run_main_application(void)
{
    UART_SendString("Integrated LDR+LED and DHT11 system\r\n");

    // Initialize all modules
    Ldr_Init();
    Lights_Init();
    DHT11_Init();

    uint32_t counter = 0;
    uint16_t temperature, humidity;
    
    while (1)
    {
        // LDR + LED automatic control (every loop)
        uint16_t ldr_value = Ldr_Read();
        
        // UART_SendString("LDR=");
        // UART_SendNumber(ldr_value);
        if (ldr_value < 800) {
            Lights_On();
//            UART_SendString(" -> LED ON\r\n");
        } else {
            Lights_Off();
//            UART_SendString(" -> LED OFF\r\n");
        }
        
        // DHT11 reading
        if (counter % 50 == 0) {
            DHT11_ErrorCode result = DHT11_Read(&temperature, &humidity);
            if (result == DHT11_OK) {
                UART_SendString("DHT11: Temp=");
                UART_SendNumber(temperature / 100);
                UART_SendString(".");
                UART_SendNumber(temperature % 100);
                UART_SendString(" C, Humidity=");
                UART_SendNumber(humidity / 100);
                UART_SendString(".");
                UART_SendNumber(humidity % 100);
                UART_SendString("%\r\n");
            } else {
                UART_SendString("DHT11 Error: ");
                UART_SendString(DHT11_GetErrorString(result));
                UART_SendString("\r\n");
            }
        }
        
         counter++;
        
        // Small delay (~50ms per iteration)
        for (volatile uint32_t i = 0; i < 240000; i++);
    }
}


