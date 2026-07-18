#ifndef OLED_H
#define OLED_H

#include <stdint.h>

uint8_t oled_init(void);
uint8_t oled_get_address(void);
void oled_clear(void);
void oled_fill(uint8_t pattern);
void oled_show_string(uint8_t row, uint8_t col, const char *str);
void oled_refresh(void);

#endif
