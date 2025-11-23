#include "fsl_adc16.h"
#include "ldr.h"

void Ldr_Init(void)
{
    adc16_config_t config;

    ADC16_GetDefaultConfig(&config);
    ADC16_Init(ADC0, &config);
    ADC16_EnableHardwareTrigger(ADC0, false);

    ADC16_DoAutoCalibration(ADC0);
}

uint16_t Ldr_Read(void)
{
    adc16_channel_config_t channel = {0};
    channel.channelNumber = 8;   // PTB0 = ADC0_SE8

    ADC16_SetChannelConfig(ADC0, 0, &channel);

    while (!(ADC16_GetChannelStatusFlags(ADC0, 0)));

    return ADC16_GetChannelConversionValue(ADC0, 0);
}
