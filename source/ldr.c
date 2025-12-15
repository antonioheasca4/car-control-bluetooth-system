#include "fsl_adc16.h"
#include "ldr.h"
#include "uart.h"

/**
 * @brief Initialize LDR sensor (ADC)
 * LDR connected to PTB0 (J10, ADC0_SE8)
 */
void Ldr_Init(void)
{
    adc16_config_t config;

    ADC16_GetDefaultConfig(&config);
    ADC16_Init(ADC0, &config);
    ADC16_EnableHardwareTrigger(ADC0, false);

    ADC16_DoAutoCalibration(ADC0);

    UART_SendString("  LDR init finish  \r\n");
}

/**
 * @brief Read LDR value from ADC
 * @return ADC value (0-4095 for 12-bit, 0-65535 for 16-bit)
 *         Higher value = More light resistance = Darker environment
 */
uint16_t Ldr_Read(void)
{
    adc16_channel_config_t channel = {0};
    channel.channelNumber = 8;   // PTB0 = ADC0_SE8

    ADC16_SetChannelConfig(ADC0, 0, &channel);

    while (!(ADC16_GetChannelStatusFlags(ADC0, 0)));

    return ADC16_GetChannelConversionValue(ADC0, 0);
}
