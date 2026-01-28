#include "bluetooth.h"
#include "uart.h"
#include "motor.h"
#include "MKL25Z4.h"

/**
 * Bluetooth Command Parser with UART0 RX Interrupt
 * 
 * Uses UART0 RX interrupt to buffer incoming bytes.
 * Commands are stored in a ring buffer and processed by main loop.
 * This prevents losing commands during sensor reads.
 */

// Ring buffer for received bytes
#define RX_BUFFER_SIZE  32
static volatile uint8_t rxBuffer[RX_BUFFER_SIZE];
static volatile uint8_t rxHead = 0;  // Write index (ISR writes here)
static volatile uint8_t rxTail = 0;  // Read index (main loop reads here)

static uint8_t currentSpeed = 0;  // Will be set from Motor_GetDefaultSpeed()

/**
 * @brief UART0 Interrupt Handler
 * Called automatically when a byte is received on UART0
 */
void UART0_IRQHandler(void)
{
    // Check if RX data register is full
    if (UART0->S1 & UART_S1_RDRF_MASK) {
        uint8_t byte = UART0->D;  // Read byte (also clears RDRF flag)
        
        // Calculate next head position
        uint8_t nextHead = (rxHead + 1) % RX_BUFFER_SIZE;
        
        // Only store if buffer not full
        if (nextHead != rxTail) {
            rxBuffer[rxHead] = byte;
            rxHead = nextHead;
        }
        // If buffer is full, byte is dropped (overflow)
    }
    
    // Clear any error flags
    if (UART0->S1 & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) {
        (void)UART0->D;  // Read to clear errors
    }
}

void Bluetooth_Init(void)
{
    // Reset buffer indices
    rxHead = 0;
    rxTail = 0;
    currentSpeed = Motor_GetDefaultSpeed();  // Get default from motor.c
    
    // Enable UART0 RX interrupt
    UART0->C2 |= UART_C2_RIE_MASK;  // Enable RX interrupt
    
    // Enable UART0 interrupt in NVIC
    NVIC_SetPriority(UART0_IRQn, 2);  // Priority 2 (medium)
    NVIC_EnableIRQ(UART0_IRQn);

    UART_SendString("  Bluetooth init (RX interrupt enabled)\r\n");
}

/**
 * @brief Check if data is available in the ring buffer
 */
bool Bluetooth_Available(void)
{
    return (rxHead != rxTail);
}

/**
 * @brief Get a byte from the ring buffer
 * @return Received byte, or 0 if buffer empty
 */
uint8_t Bluetooth_GetByte(void)
{
    if (!Bluetooth_Available()) {
        return 0;
    }
    
    uint8_t byte = rxBuffer[rxTail];
    rxTail = (rxTail + 1) % RX_BUFFER_SIZE;
    return byte;
}

/**
 * @brief Get number of bytes in buffer
 */
uint8_t Bluetooth_GetBufferCount(void)
{
    if (rxHead >= rxTail) {
        return rxHead - rxTail;
    } else {
        return RX_BUFFER_SIZE - rxTail + rxHead;
    }
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
