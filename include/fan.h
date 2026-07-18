#ifndef FAN_H
#define FAN_H

#include <stdint.h>

typedef enum
{
    FAN_OFF = 0,
    FAN_ON  = 1
} FanState;

void fan_init(void);
void fan_set(FanState state);
FanState fan_get(void);

#endif
