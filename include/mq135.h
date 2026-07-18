#ifndef MQ135_H
#define MQ135_H

#include <stdint.h>

void mq135_init(void);
uint16_t mq135_read_raw(void);
uint16_t mq135_read_average(void);

#endif
