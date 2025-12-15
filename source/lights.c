#include "lights.h"
#include "fsl_gpio.h"
#include "uart.h"

// LED connected to PTC1 (J10 on FRDM-KL25Z)
#define LED_GPIO GPIOC
#define LED_PIN 1


void Lights_Init(void)
{
    gpio_pin_config_t cfg = {kGPIO_DigitalOutput, 0};
    GPIO_PinInit(LED_GPIO, LED_PIN, &cfg);

    UART_SendString("  Lights init finish  \r\n");
}

void Lights_On(void)
{
    GPIO_SetPinsOutput(LED_GPIO, 1 << LED_PIN);
}

void Lights_Off(void)
{
    GPIO_ClearPinsOutput(LED_GPIO, 1 << LED_PIN);
}

void Lights_Auto(uint16_t ldrValue)
{
    const uint16_t threshold = 1500; // ajustare prag
    
    // LDR Pull-Down configuration (3.3V → LDR → PTB0 → 10kΩ → GND):
    // Bright light → LDR low resistance → HIGH voltage → HIGH ADC value
    // Darkness → LDR high resistance → LOW voltage → LOW ADC value
    if (ldrValue < threshold)
        Lights_On();   // Low ADC = Dark → LED ON
    else
        Lights_Off();  // High ADC = Bright → LED OFF
}
