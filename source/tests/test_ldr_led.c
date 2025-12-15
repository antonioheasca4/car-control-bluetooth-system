#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "../uart.h"

#include "../ldr.h"
#include "../lights.h"

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
        if (val < 1500) {
            UART_SendString(" -> LED ON (dark)\r\n");
        } else {
            UART_SendString(" -> LED OFF (bright)\r\n");
        }

        Lights_Auto(val);

        // Delay ~500ms between readings
        for (volatile uint32_t i = 0; i < 2400000; i++);
    }
}
