#include "fan.h"
#include "app_config.h"
#include "stm32f1xx.h"

static FanState s_state = FAN_OFF;

#if ENABLE_FAN_OUTPUT == 1
static void gpio_config_output_pp(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    *cr &= ~(0xFU << shift);
    *cr |=  (0x2U << shift);
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
#endif

void fan_init(void)
{
#if ENABLE_FAN_OUTPUT == 1
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    gpio_config_output_pp(FAN_IA_PORT, FAN_IA_PIN);
    gpio_config_output_pp(FAN_IB_PORT, FAN_IB_PIN);

    fan_set(FAN_OFF);
#else
    /* Fail-safe public default: do not enable GPIOB and do not reconfigure
     * either candidate motor-driver input as an output. */
    s_state = FAN_OFF;
#endif
}

void fan_set(FanState state)
{
#if ENABLE_FAN_OUTPUT == 1
    if (state == FAN_ON)
    {
        gpio_write_pin(FAN_IA_PORT, FAN_IA_PIN, 1U);
        gpio_write_pin(FAN_IB_PORT, FAN_IB_PIN, 0U);
    }
    else
    {
        gpio_write_pin(FAN_IA_PORT, FAN_IA_PIN, 0U);
        gpio_write_pin(FAN_IB_PORT, FAN_IB_PIN, 0U);
    }

    s_state = state;
#else
    (void)state;
    s_state = FAN_OFF;
#endif
}

FanState fan_get(void)
{
    return s_state;
}
