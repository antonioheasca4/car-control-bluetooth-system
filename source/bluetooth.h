#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Bluetooth Command Module
 * 
 * Uses UART0 (PTA1 RX, PTA2 TX) for HC-05/HC-06 Bluetooth module
 * 
 * Command Protocol (single character):
 *   Movement:
 *     'F' or 'W' - Forward
 *     'B' or 'X' - Backward
 *     'L' or 'A' - Turn Left
 *     'R' or 'D' - Turn Right
 *     'S' or ' ' - Stop
 *   
 *   Lights:
 *     'O' - Lights ON
 *     'P' - Lights OFF (Power off)
 *     'M' - Toggle Auto-lights Mode
 *   
 *   Sensors:
 *     'T' - Get Temperature
 *     'H' - Get Humidity
 *     'U' - Get Ultrasonic Distance
 *     'I' - Get Info (all sensors)
 *   
 *   Speed:
 *     '1'-'9' - Set speed (10%-90%)
 */

typedef enum {
    CMD_NONE = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_STOP,
    CMD_LIGHTS_ON,
    CMD_LIGHTS_OFF,
    CMD_LIGHTS_AUTO,
    CMD_GET_TEMP,
    CMD_GET_HUMIDITY,
    CMD_GET_DISTANCE,
    CMD_GET_INFO,
    CMD_SET_SPEED,
    CMD_UNKNOWN
} BluetoothCommand;

/**
 * @brief Initialize Bluetooth UART communication
 * @note UART0 should already be initialized by BOARD_InitDebugConsole()
 */
void Bluetooth_Init(void);

/**
 * @brief Check if data is available from Bluetooth
 * @return true if byte available to read
 */
bool Bluetooth_Available(void);

/**
 * @brief Get a single byte from Bluetooth
 * @return Received byte, or 0 if none available
 */
uint8_t Bluetooth_GetByte(void);

/**
 * @brief Parse received byte and return command
 * @return Command enum value
 */
BluetoothCommand Bluetooth_GetCommand(void);

/**
 * @brief Send a string via Bluetooth
 * @param str Null-terminated string to send
 */
void Bluetooth_SendString(const char* str);

/**
 * @brief Send a number via Bluetooth
 * @param num Number to send
 */
void Bluetooth_SendNumber(uint32_t num);

/**
 * @brief Send formatted sensor data
 * @param label Sensor name
 * @param value Sensor value
 * @param unit Unit string (e.g., "cm", "C", "%")
 */
void Bluetooth_SendSensorData(const char* label, uint32_t value, const char* unit);

/**
 * @brief Get current speed setting (0-100)
 * @return Speed percentage
 */
uint8_t Bluetooth_GetSpeed(void);

#endif // BLUETOOTH_H
