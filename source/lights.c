#include "lights.h"
#include "fsl_gpio.h"

#define LED_GPIO GPIOB
#define LED_PIN  18

void Lights_Init(void)
{
    gpio_pin_config_t cfg = {kGPIO_DigitalOutput, 0};
    GPIO_PinInit(LED_GPIO, LED_PIN, &cfg);
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

    if (ldrValue > threshold)
        Lights_On();
    else
        Lights_Off();
}
