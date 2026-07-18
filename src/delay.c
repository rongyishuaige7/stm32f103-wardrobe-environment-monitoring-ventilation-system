#include "delay.h"
#include "stm32f1xx.h"

static uint32_t s_ticks_per_us = 8U;
static uint32_t s_fake_ms = 0U;

void delay_init(void)
{
    s_ticks_per_us = SystemCoreClock / 1000000U;
    if (s_ticks_per_us == 0U)
    {
        s_ticks_per_us = 1U;
    }

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0x00FFFFFFU;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t ms)
{
    while (ms-- > 0U)
    {
        delay_us(1000U);
        s_fake_ms++;
    }
}

void delay_us(uint32_t us)
{
    uint32_t target_ticks = us * s_ticks_per_us;
    uint32_t reload = 0x01000000U;
    uint32_t previous = SysTick->VAL;
    uint32_t elapsed = 0U;

    while (elapsed < target_ticks)
    {
        uint32_t current = SysTick->VAL;

        if (current <= previous)
        {
            elapsed += previous - current;
        }
        else
        {
            elapsed += previous + (reload - current);
        }
        previous = current;
    }
}

uint32_t millis(void)
{
    return s_fake_ms;
}
