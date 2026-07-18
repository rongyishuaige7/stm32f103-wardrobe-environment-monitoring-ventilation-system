#include "mq135.h"
#include "app_config.h"
#include "delay.h"
#include "stm32f1xx.h"

static void gpio_config_analog(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    *cr &= ~(0xFU << shift);
}

void mq135_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    gpio_config_analog(MQ135_PORT, MQ135_PIN);

    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    ADC1->CR1 = 0U;
    ADC1->CR2 = ADC_CR2_ADON;
    delay_ms(2U);
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0U)
    {
    }
    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0U)
    {
    }

    ADC1->SMPR2 &= ~(7U << (3U * MQ135_PIN));
    ADC1->SMPR2 |=  (7U << (3U * MQ135_PIN));
    ADC1->SQR1 = 0U;
    ADC1->SQR3 = 0U;
    ADC1->CR2 |= ADC_CR2_EXTTRIG;
}

uint16_t mq135_read_raw(void)
{
    ADC1->SQR3 = MQ135_PIN;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
    }

    return (uint16_t)ADC1->DR;
}

uint16_t mq135_read_average(void)
{
    uint32_t sum = 0U;
    uint32_t i;

    for (i = 0U; i < MQ135_SAMPLE_COUNT; i++)
    {
        sum += mq135_read_raw();
        delay_ms(2U);
    }

    return (uint16_t)(sum / MQ135_SAMPLE_COUNT);
}
