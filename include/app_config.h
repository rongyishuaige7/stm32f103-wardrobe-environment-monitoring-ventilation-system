#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "stm32f1xx.h"

#ifndef ENABLE_FAN_OUTPUT
#define ENABLE_FAN_OUTPUT 0
#endif

#ifndef ENABLE_STARTUP_FAN_TEST
#define ENABLE_STARTUP_FAN_TEST 0
#endif

#if (ENABLE_FAN_OUTPUT != 0) && (ENABLE_FAN_OUTPUT != 1)
#error "ENABLE_FAN_OUTPUT must be exactly 0 or 1"
#endif

#if (ENABLE_STARTUP_FAN_TEST != 0) && (ENABLE_STARTUP_FAN_TEST != 1)
#error "ENABLE_STARTUP_FAN_TEST must be exactly 0 or 1"
#endif

#if (ENABLE_STARTUP_FAN_TEST == 1) && (ENABLE_FAN_OUTPUT != 1)
#error "Startup fan test requires ENABLE_FAN_OUTPUT=1"
#endif

/* Control thresholds: change these values first when tuning behavior. */
#define HUMIDITY_ON_THRESHOLD      75U
#define HUMIDITY_OFF_THRESHOLD     70U
#define MQ_RAW_ON_THRESHOLD        2300U
#define MQ_RAW_OFF_THRESHOLD       2000U

#define FAN_IA_PORT GPIOB
#define FAN_IA_PIN  0U
#define FAN_IB_PORT GPIOB
#define FAN_IB_PIN  1U

#define DHT11_PORT   GPIOB
#define DHT11_PIN    12U

#define MQ135_PORT   GPIOA
#define MQ135_PIN    0U

#define OLED_SCL_PORT GPIOB
#define OLED_SCL_PIN  6U
#define OLED_SDA_PORT GPIOB
#define OLED_SDA_PIN  7U
#define OLED_I2C_ADDRESS 0x78U

#define MQ135_SAMPLE_COUNT          16U

#endif
