#include "uart.h"
#include "stm32f1xx.h"
#include <stdio.h>

static void gpio_config_output_af_pp(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    *cr &= ~(0xFU << shift);
    *cr |=  (0xBU << shift);
}

static void gpio_config_input_floating(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;

    *cr &= ~(0xFU << shift);
    *cr |=  (0x4U << shift);
}

void uart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;

    gpio_config_output_af_pp(GPIOA, 9U);
    gpio_config_input_floating(GPIOA, 10U);

    {
        uint32_t usartdiv_x16 = (SystemCoreClock + 57600U) / 115200U;
        uint32_t mantissa = usartdiv_x16 / 16U;
        uint32_t fraction = usartdiv_x16 % 16U;
        USART1->BRR = (mantissa << 4) | fraction;
    }
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void uart1_write_char(char ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }
    USART1->DR = (uint16_t)ch;
}

void uart1_write_string(const char *s)
{
    while (*s != '\0')
    {
        uart1_write_char(*s++);
    }
}

int fputc(int ch, FILE *f)
{
    (void)f;
    uart1_write_char((char)ch);
    return ch;
}
