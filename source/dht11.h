#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stdbool.h>

// DHT11 sensor type
#define DHT11_SENSOR_PERIOD_MS    1000
  /*!< can only read sensor values with 1 Hz! */

/*! Error codes */
typedef enum {
  DHT11_OK,           /*!< OK */
  DHT11_NO_PULLUP,    /*!< no pull-up present */
  DHT11_NO_ACK_0,     /*!< no 0 acknowledge detected */
  DHT11_NO_ACK_1,     /*!< no 1 acknowledge detected */
  DHT11_NO_DATA_0,    /*!< low level expected during data transmission */
  DHT11_NO_DATA_1,    /*!< high level expected during data transmission */
  DHT11_BAD_CRC,      /*!< bad CRC */
} DHT11_ErrorCode;

// Function prototypes
void DHT11_Init(void);
DHT11_ErrorCode DHT11_Read(uint16_t *temperatureCentigrade, uint16_t *humidityCentipercent);
const char *DHT11_GetErrorString(DHT11_ErrorCode code);

#endif // DHT11_H
