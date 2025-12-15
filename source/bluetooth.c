#include "bluetooth.h"
#include "uart.h"
#include "MKL25Z4.h"

/**
 * Bluetooth Command Parser
 * 
 * Uses UART0 which is connected to the Bluetooth module (HC-05/HC-06)
 * and also used for debug output via USB serial.
 * 
 * Command format: Single ASCII character
 * Response format: Text with \r\n line ending
 */

static uint8_t currentSpeed = 70;  // Default speed 70%

void Bluetooth_Init(void)
{
    // UART0 is already initialized by BOARD_InitDebugConsole()
    // Just reset speed to default
    currentSpeed = 70;

    UART_SendString("  Bluetooth init finish  \r\n");
}

bool Bluetooth_Available(void)
{
    // Check if UART0 receive data register is full
    return (UART0->S1 & UART_S1_RDRF_MASK) != 0;
}

uint8_t Bluetooth_GetByte(void)
{
    if (!Bluetooth_Available()) {
        return 0;
    }
    return UART0->D;
}

BluetoothCommand Bluetooth_GetCommand(void)
{
    if (!Bluetooth_Available()) {
        return CMD_NONE;
    }
    
    uint8_t byte = Bluetooth_GetByte();
    
    // Convert to uppercase for easier parsing
    if (byte >= 'a' && byte <= 'z') {
        byte = byte - 'a' + 'A';
    }
    
    // Speed setting: digits 1-9 set speed 10%-90%
    if (byte >= '1' && byte <= '9') {
        currentSpeed = (byte - '0') * 10;
        return CMD_SET_SPEED;
    }
    
    // Command parsing
    switch (byte) {
        case 'F':
        case 'W':   // Alternative: W for forward (WASD style)
            return CMD_FORWARD;
            
        case 'B':
        case 'X':   // Alternative: X for backward
            return CMD_BACKWARD;
            
        case 'L':
        case 'A':   // Alternative: A for left (WASD style)
            return CMD_LEFT;
            
        case 'R':
        case 'D':   // Alternative: D for right (WASD style)
            return CMD_RIGHT;
            
        case 'S':
        case ' ':   // Space for stop
            return CMD_STOP;
            
        case 'O':   // Lights On
            return CMD_LIGHTS_ON;
            
        case 'P':   // Lights Off (P = Power off lights)
            return CMD_LIGHTS_OFF;
            
        case 'M':   // Auto-light Mode toggle
            return CMD_LIGHTS_AUTO;
            
        case 'T':   // Temperature
            return CMD_GET_TEMP;
            
        case 'H':   // Humidity
            return CMD_GET_HUMIDITY;
            
        case 'U':   // Ultrasonic distance
            return CMD_GET_DISTANCE;
            
        case 'I':   // Info (all sensors)
            return CMD_GET_INFO;
            
        case '\r':
        case '\n':
        case '\t':
        case '\0':
            // Ignore line endings and control characters
            return CMD_NONE;
            
        default:
            // Ignore any other control characters (0-31)
            if (byte < 32) {
                return CMD_NONE;
            }
            return CMD_UNKNOWN;
    }
}

void Bluetooth_SendString(const char* str)
{
    UART_SendString(str);
}

void Bluetooth_SendNumber(uint32_t num)
{
    UART_SendNumber(num);
}

void Bluetooth_SendSensorData(const char* label, uint32_t value, const char* unit)
{
    Bluetooth_SendString(label);
    Bluetooth_SendString(": ");
    Bluetooth_SendNumber(value);
    Bluetooth_SendString(unit);
    Bluetooth_SendString("\r\n");
}

uint8_t Bluetooth_GetSpeed(void)
{
    return currentSpeed;
}
