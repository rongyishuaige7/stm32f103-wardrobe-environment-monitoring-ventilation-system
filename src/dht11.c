#include "dht11.h"
#include "delay.h"
#include "stm32f1xx.h"
#include "app_config.h"

#define DHT11_ERR_NONE       0U
#define DHT11_ERR_ARG        1U
#define DHT11_ERR_RESP_LOW   2U
#define DHT11_ERR_RESP_HIGH  3U
#define DHT11_ERR_BIT_START  4U
#define DHT11_ERR_BIT_HIGH   5U
#define DHT11_ERR_CHECKSUM   6U
#define DHT11_ERR_ZERO_DATA  7U

#define DHT11_START_LOW_MS       20U
#define DHT11_RELEASE_US         30U
#define DHT11_TIMEOUT_US         120U
/* Typical DHT11 high pulses are about 26-28 us for 0 and about 70 us for 1.
 * A midpoint avoids treating an ordinary zero pulse as one. This remains a
 * source-level correction until the exact sensor and current commit are tested.
 */
#define DHT11_BIT_ONE_US         50U

static uint8_t s_last_error = DHT11_ERR_NONE;

static void gpio_config_output_od(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    *cr &= ~(0xFU << shift);
    *cr |=  (0x6U << shift);
}

static void gpio_config_input_pullup(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    port->BSRR = (uint32_t)(1U << pin);
    *cr &= ~(0xFU << shift);
    *cr |=  (0x8U << shift);
}

static void gpio_write_pin(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value != 0U)
    {
        port->BSRR = (uint32_t)(1U << pin);
    }
    else
    {
        port->BRR = (uint32_t)(1U << pin);
    }
}

static uint8_t gpio_read_pin(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR & (1U << pin)) ? 1U : 0U;
}

static int wait_level_us(uint8_t level, uint16_t timeout_us)
{
    while (gpio_read_pin(DHT11_PORT, DHT11_PIN) != level)
    {
        if (timeout_us-- == 0U)
        {
            return -1;
        }
        delay_us(1U);
    }

    return 0;
}

static int read_bit(uint8_t *bit)
{
    uint16_t high_us = 0U;

    if (wait_level_us(1U, DHT11_TIMEOUT_US) != 0)
    {
        s_last_error = DHT11_ERR_BIT_START;
        return -1;
    }

    while (gpio_read_pin(DHT11_PORT, DHT11_PIN) != 0U)
    {
        if (high_us++ >= DHT11_TIMEOUT_US)
        {
            s_last_error = DHT11_ERR_BIT_HIGH;
            return -1;
        }
        delay_us(1U);
    }

    *bit = (uint8_t)(high_us > DHT11_BIT_ONE_US ? 1U : 0U);
    return 0;
}

static int read_byte(uint8_t *value)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; i++)
    {
        uint8_t bit;

        if (read_bit(&bit) != 0)
        {
            return -1;
        }

        data <<= 1U;
        data |= bit;
    }

    *value = data;
    return 0;
}

void dht11_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    gpio_config_input_pullup(DHT11_PORT, DHT11_PIN);
    delay_ms(1000U);
    s_last_error = DHT11_ERR_NONE;
}

int dht11_read(Dht11Data *out)
{
    uint8_t buf[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t i;

    if (out == 0)
    {
        s_last_error = DHT11_ERR_ARG;
        return -1;
    }

    s_last_error = DHT11_ERR_NONE;

    gpio_config_output_od(DHT11_PORT, DHT11_PIN);
    gpio_write_pin(DHT11_PORT, DHT11_PIN, 0U);
    delay_ms(DHT11_START_LOW_MS);

    gpio_write_pin(DHT11_PORT, DHT11_PIN, 1U);
    gpio_config_input_pullup(DHT11_PORT, DHT11_PIN);
    delay_us(DHT11_RELEASE_US);

    if (wait_level_us(0U, DHT11_TIMEOUT_US) != 0)
    {
        s_last_error = DHT11_ERR_RESP_LOW;
        return -1;
    }

    if (wait_level_us(1U, DHT11_TIMEOUT_US) != 0)
    {
        s_last_error = DHT11_ERR_RESP_HIGH;
        return -1;
    }

    if (wait_level_us(0U, DHT11_TIMEOUT_US) != 0)
    {
        s_last_error = DHT11_ERR_RESP_LOW;
        return -1;
    }

    for (i = 0U; i < 5U; i++)
    {
        if (read_byte(&buf[i]) != 0)
        {
            return -1;
        }
    }

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
    {
        s_last_error = DHT11_ERR_CHECKSUM;
        return -1;
    }

    if ((buf[0] == 0U) && (buf[1] == 0U) && (buf[2] == 0U) && (buf[3] == 0U) && (buf[4] == 0U))
    {
        s_last_error = DHT11_ERR_ZERO_DATA;
        return -1;
    }

    out->humidity = buf[0];
    out->humidity_decimal = buf[1];
    out->temperature = buf[2];
    out->temperature_decimal = buf[3];
    return 0;
}

uint8_t dht11_last_error(void)
{
    return s_last_error;
}
