#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#include "ldr.h"
#include "lights.h"

void run_test_ldr_led(void)
{
    PRINTF("\r\n===== TEST LDR + LED =====\r\n");

    Ldr_Init();
    Lights_Init();

    while (1)
    {
        uint16_t val = Ldr_Read();

        PRINTF("LDR value: %d\r\n", val);

        Lights_Auto(val);

        SDK_DelayAtLeastUs(200000, CLOCK_GetFreq(kCLOCK_CoreSysClk)); // 200ms
    }
}
