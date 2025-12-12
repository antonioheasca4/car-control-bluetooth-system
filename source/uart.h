#ifndef UART_H
#define UART_H

#include <stdint.h>

void UART_SendByte(uint8_t byte);
void UART_SendString(const char *str);
void UART_SendNumber(uint32_t num);

#endif
