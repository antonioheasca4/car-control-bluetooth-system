#include "MKL25Z4.h"
#include "uart.h"

// UART helper functions
void UART_SendByte(uint8_t byte)
{
    while (!(UART0->S1 & UART_S1_TDRE_MASK));
    UART0->D = byte;
}

void UART_SendString(const char* str)
{
    while (*str) {
        UART_SendByte(*str++);
    }
}

void UART_SendNumber(uint32_t num)
{
    char buffer[12];
    int i = 0;

    if (num == 0) {
        UART_SendByte('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) {
        UART_SendByte(buffer[--i]);
    }
}
