#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "fsl_lpsci.h"
#include "dht11.h"
#include "../uart.h"

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
