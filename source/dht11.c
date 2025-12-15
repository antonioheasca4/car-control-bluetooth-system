#include "dht11.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_clock.h"
#include "board.h"
#include "MKL25Z4.h"
#include "uart.h"

// DHT11 connected to PTD4 (J1-D4 on FRDM-KL25Z)
// Using 3-pin DHT11 module with built-in 10kΩ pull-up resistor
#define DHT11_GPIO      GPIOD
#define DHT11_PORT      PORTD
#define DHT11_PIN       4U
#define DHT11_PIN_MASK  (1U << DHT11_PIN)

// Timing helpers - accurate microsecond delays for 48 MHz core clock
static void DHT11_DelayUs(uint32_t us)
{
    // At 48 MHz: 48 cycles per microsecond
    // Each loop iteration: ~3 cycles (sub + branch)
    // So: us * 16 iterations
    volatile uint32_t cycles = us * 16;
    while (cycles--) {
        __asm("nop");
        __asm("nop");
        __asm("nop");
    }
}

static void DHT11_DelayMs(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        DHT11_DelayUs(1000);
    }
}

// GPIO pin control
static void DHT11_SetPinOutput(void)
{
    DHT11_GPIO->PDDR |= DHT11_PIN_MASK;  // Set as output
}

static void DHT11_SetPinInput(void)
{
    DHT11_GPIO->PDDR &= ~DHT11_PIN_MASK; // Set as input
}

static void DHT11_PinWrite(uint8_t value)
{
    if (value) {
        DHT11_GPIO->PSOR = DHT11_PIN_MASK; // Set high
    } else {
        DHT11_GPIO->PCOR = DHT11_PIN_MASK; // Set low
    }
}

static uint8_t DHT11_PinRead(void)
{
    return (DHT11_GPIO->PDIR & DHT11_PIN_MASK) ? 1 : 0;
}

/**
 * @brief Initialize DHT11 sensor
 */
void DHT11_Init(void)
{
	// Activeaza pull-up intern pe PTD4
	PORTD->PCR[DHT11_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	// PE = Pull Enable, PS = Pull Select (pull-up)

    // Set as input initially (idle state)
    DHT11_SetPinInput();
    
    // Wait for sensor stabilization (1 second after power-on)
    DHT11_DelayMs(1000);

    UART_SendString("  DHT11 init finish  \r\n");
}

const char *DHT11_GetErrorString(DHT11_ErrorCode code)
{
    switch(code) {
        case DHT11_OK:        return "OK";
        case DHT11_NO_PULLUP: return "NO_PULLUP";
        case DHT11_NO_ACK_0:  return "NO_ACK_0";
        case DHT11_NO_ACK_1:  return "NO_ACK_1";
        case DHT11_NO_DATA_0: return "NO_DATA_0";
        case DHT11_NO_DATA_1: return "NO_DATA_1";
        case DHT11_BAD_CRC:   return "BAD_CRC";
        default:              return "UNKNOWN";
    }
}

DHT11_ErrorCode DHT11_Read(uint16_t *temperatureCentigrade, uint16_t *humidityCentipercent)
{
    int cntr;
    int loopBits;
    uint8_t buffer[5];
    int i;
    int data;
 
    /* init buffer */
    for(i=0; i<sizeof(buffer); i++) {
        buffer[i] = 0;
    }
    
    /* Disable interrupts for critical timing section */
    __disable_irq();
    
    /* set to input and check if the signal gets pulled up */
    DHT11_SetPinInput();
    DHT11_DelayUs(50);
    if(DHT11_PinRead() == 0) {
        __enable_irq();
        return DHT11_NO_PULLUP;
    }
    
    /* send start signal */
    DHT11_SetPinOutput();
    DHT11_PinWrite(0);
    DHT11_DelayMs(18); /* keep signal low for at least 18 ms */
    DHT11_SetPinInput();
    DHT11_DelayUs(50);
    
    /* check for acknowledge signal */
    if (DHT11_PinRead() != 0) { /* signal must be pulled low by the sensor */
        __enable_irq();
        return DHT11_NO_ACK_0;
    }
    
    /* wait max 100 us for the ack signal from the sensor */
    cntr = 18;
    while(DHT11_PinRead() == 0) { /* wait until signal goes up */
        DHT11_DelayUs(5);
        if (--cntr == 0) {
            __enable_irq();
            return DHT11_NO_ACK_1; /* signal should be up for the ACK here */
        }
    }
    
    /* wait until it goes down again, end of ack sequence */
    cntr = 18;
    while(DHT11_PinRead() != 0) { /* wait until signal goes down */
        DHT11_DelayUs(5);
        if (--cntr == 0) {
            __enable_irq();
            return DHT11_NO_ACK_0; /* signal should be down to zero again here */
        }
    }
    
    /* now read the 40 bit data */
    i = 0;
    data = 0;
    loopBits = 40;
    do {
        cntr = 11; /* wait max 55 us */
        while(DHT11_PinRead() == 0) {
            DHT11_DelayUs(5);
            if (--cntr == 0) {
                __enable_irq();
                return DHT11_NO_DATA_0;
            }
        }
        cntr = 15; /* wait max 75 us */
        while(DHT11_PinRead() != 0) {
            DHT11_DelayUs(5);
            if (--cntr == 0) {
                __enable_irq();
                return DHT11_NO_DATA_1;
            }
        }
        data <<= 1; /* next data bit */
        if (cntr < 10) { /* data signal high > 30 us ==> data bit 1 */
            data |= 1;
        }
        if ((loopBits & 0x7) == 1) { /* next byte */
            buffer[i] = data;
            i++;
            data = 0;
        }
    } while(--loopBits != 0);
    
    /* Re-enable interrupts */
    __enable_irq();
 
    /* now we have the 40 bit (5 bytes) data:
     * byte 0: humidity integer data
     * byte 1: humidity decimal data (not used for DTH11, always zero)
     * byte 2: temperature integer data
     * byte 3: temperature fractional data (not used for DTH11, always zero)
     * byte 4: checksum, the sum of byte 0 + 1 + 2 + 3
     */
    /* test CRC */
    if (buffer[0] + buffer[1] + buffer[2] + buffer[3] != buffer[4]) {
        return DHT11_BAD_CRC;
    }
    
    /* store data values for caller */
    *humidityCentipercent = ((int)buffer[0]) * 100;
    *temperatureCentigrade = ((int)buffer[2]) * 100;
    
    return DHT11_OK;
}
