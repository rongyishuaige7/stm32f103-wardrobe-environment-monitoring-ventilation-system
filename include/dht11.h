#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

typedef struct
{
    uint8_t temperature;
    uint8_t temperature_decimal;
    uint8_t humidity;
    uint8_t humidity_decimal;
} Dht11Data;

void dht11_init(void);
int dht11_read(Dht11Data *out);
uint8_t dht11_last_error(void);

#endif
