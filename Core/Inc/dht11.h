#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

typedef struct {
    uint8_t humidity;      /* %RH */
    uint8_t hum_dec;
    uint8_t temperature;   /* °C */
    uint8_t temp_dec;
} DHT11_Data;

typedef enum {
    DHT11_OK = 0,
    DHT11_ERR_NO_RESPONSE,
    DHT11_ERR_TIMEOUT,
    DHT11_ERR_CHECKSUM
} DHT11_Status;

void         DHT11_Init(void);
DHT11_Status DHT11_Read(DHT11_Data *data);                          /* đọc + giải mã */
void         DHT11_ToString(DHT11_Status st, const DHT11_Data *d,
                            char *out, uint32_t size);              /* đổi sang chuỗi */

#endif