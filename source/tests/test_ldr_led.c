#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_lpsci.h"
#include "../uart.h"




#include "ldr.h"
#include "lights.h"



void run_test_ldr_led(void)
{
    // Test UART directly
    UART_SendString("\r\n===== TEST LDR + LED =====\r\n");
    UART_SendString("UART Direct Test - Starting...\r\n");

    Ldr_Init();
    Lights_Init();
    
    UART_SendString("LDR and LED initialized\r\n");

    while (1)
    {
        uint16_t val = Ldr_Read();

        UART_SendString("LDR=");
        // Simple number to string conversion
        char numStr[10];
        sprintf(numStr, "%d", val);
        UART_SendString(numStr);
        UART_SendString("\r\n");

        Lights_Auto(val);

        for (volatile int i = 0; i < 1000000; i++);

    }
}
