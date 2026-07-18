#include "stm32f1xx.h"
#include "delay.h"
#include "uart.h"
#include "dht11.h"
#include "mq135.h"
#include "fan.h"
#include "oled.h"
#include "app_config.h"

static void debug_led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(0xFU << 20U);
    GPIOC->CRH |=  (0x2U << 20U);
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

static void debug_led_on(void)
{
    GPIOC->BRR = GPIO_BRR_BR13;
}

static void debug_led_off(void)
{
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

static void debug_led_toggle(void)
{
    if ((GPIOC->ODR & GPIO_ODR_ODR13) != 0U)
    {
        debug_led_on();
    }
    else
    {
        debug_led_off();
    }
}

static void debug_rough_delay(void)
{
    volatile uint32_t i;

    for (i = 0U; i < 3000000U; i++)
    {
    }
}

static void debug_startup_blink(void)
{
    uint8_t i;

    for (i = 0U; i < 6U; i++)
    {
        debug_led_toggle();
        debug_rough_delay();
    }
    debug_led_on();
}

static void gpio_init_base(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
    AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG;
}

static void append_u16(char *buf, uint16_t value)
{
    char temp[6];
    uint8_t i = 0U;
    uint8_t j;

    if (value == 0U)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (value > 0U)
    {
        temp[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    for (j = 0U; j < i; j++)
    {
        buf[j] = temp[i - 1U - j];
    }
    buf[i] = '\0';
}

static void uart_write_u16(uint16_t value)
{
    char buf[6];

    append_u16(buf, value);
    uart1_write_string(buf);
}

static void uart_write_decimal(uint16_t integer, uint8_t decimal)
{
    uart_write_u16(integer);
    uart1_write_string(".");
    uart_write_u16((uint16_t)(decimal % 10U));
}

static void uart_log_status(uint16_t mq_raw, uint16_t demo_index, FanState fan, const char *reason)
{
    uart1_write_string("MQ135 raw=");
    uart_write_u16(mq_raw);
    uart1_write_string(" demo_index=");
    uart_write_u16(demo_index);
    uart1_write_string("/100 fan=");
    uart1_write_string(fan == FAN_ON ? "ON" : "OFF");
    uart1_write_string(" reason=");
    uart1_write_string(reason);
}

static void uart_log_dht(uint8_t ok, const Dht11Data *dht)
{
    if (ok == 0U)
    {
        uart1_write_string(" DHT11=FAIL E=");
        uart_write_u16(dht11_last_error());
    }
    else
    {
        uart1_write_string(" T=");
        uart_write_decimal(dht->temperature, dht->temperature_decimal);
        uart1_write_string("C H=");
        uart_write_decimal(dht->humidity, dht->humidity_decimal);
        uart1_write_string("%");
    }
}

static void make_line(char *line, const char *prefix, uint16_t value, const char *suffix)
{
    uint8_t idx = 0U;
    uint8_t i = 0U;
    char number[6];

    while (prefix[i] != '\0')
    {
        line[idx++] = prefix[i++];
    }

    append_u16(number, value);
    i = 0U;
    while (number[i] != '\0')
    {
        line[idx++] = number[i++];
    }

    i = 0U;
    while (suffix[i] != '\0')
    {
        line[idx++] = suffix[i++];
    }

    line[idx] = '\0';
}

static void make_decimal_line(char *line, const char *prefix, uint16_t integer, uint8_t decimal, const char *suffix)
{
    uint8_t idx = 0U;
    uint8_t i = 0U;
    char number[6];

    while (prefix[i] != '\0')
    {
        line[idx++] = prefix[i++];
    }

    append_u16(number, integer);
    i = 0U;
    while (number[i] != '\0')
    {
        line[idx++] = number[i++];
    }

    line[idx++] = '.';
    line[idx++] = (char)('0' + (decimal % 10U));

    i = 0U;
    while (suffix[i] != '\0')
    {
        line[idx++] = suffix[i++];
    }

    line[idx] = '\0';
}

static const char *fan_reason_short(const char *reason)
{
    if (reason == 0)
    {
        return "--";
    }
    if ((reason[0] == 'm') && (reason[1] == 'q') && (reason[2] == '+'))
    {
        return "M+H";
    }
    if ((reason[0] == 'm') && (reason[1] == 'q') && (reason[2] == '\0'))
    {
        return "MQ";
    }
    if ((reason[0] == 'h') && (reason[1] == 'o'))
    {
        return "HOLD";
    }
    if (reason[0] == 'h')
    {
        return "HUM";
    }
    if (reason[0] == 'n')
    {
        return "NOR";
    }
    if ((reason[0] == 'm') && (reason[1] == 'q') && (reason[2] == '-'))
    {
        return "MQN";
    }
    return "--";
}

static void oled_show_status_screen(const Dht11Data *dht, uint8_t dht_ok, uint16_t mq_raw, uint16_t demo_index, FanState fan, const char *reason)
{
    char line[22];
    const char *reason_short = fan_reason_short(reason);

    oled_clear();

    if (dht_ok != 0U)
    {
        make_decimal_line(line, "Temp:", dht->temperature, dht->temperature_decimal, "C");
    }
    else
    {
        oled_show_string(0U, 0U, "Temp:--.-C");
        line[0] = '\0';
    }
    if (line[0] != '\0')
    {
        oled_show_string(0U, 0U, line);
    }

    if (dht_ok != 0U)
    {
        make_decimal_line(line, "Humi:", dht->humidity, dht->humidity_decimal, "%");
    }
    else
    {
        oled_show_string(2U, 0U, "Humi:--.-%");
        line[0] = '\0';
    }
    if (line[0] != '\0')
    {
        oled_show_string(2U, 0U, line);
    }

    make_line(line, "MQ:", mq_raw, "");
    oled_show_string(4U, 0U, line);

    make_line(line, "Idx:", demo_index, "");
    oled_show_string(4U, 10U, line);

    oled_show_string(6U, 0U, fan == FAN_ON ? "Fan:ON " : "Fan:OFF");
    oled_show_string(6U, 10U, reason_short);
    oled_refresh();
}

static void oled_show_startup_countdown(uint8_t seconds_left)
{
    char line[22];

    oled_clear();
    oled_show_string(0U, 0U, "Wardrobe Air");
    oled_show_string(2U, 0U, "Starting...");
    make_line(line, "Ready in ", seconds_left, "s");
    oled_show_string(4U, 0U, line);
#if ENABLE_STARTUP_FAN_TEST == 1
    oled_show_string(6U, 0U, "Fan test next");
#else
    oled_show_string(6U, 0U, "Safe output off");
#endif
    oled_refresh();
}

#if ENABLE_STARTUP_FAN_TEST == 1
static void oled_show_fan_test(uint8_t seconds_left)
{
    char line[22];

    oled_clear();
    oled_show_string(0U, 0U, "Wardrobe Air");
    oled_show_string(2U, 0U, "Fan Testing");
    make_line(line, "Remain ", seconds_left, "s");
    oled_show_string(4U, 0U, line);
    oled_show_string(6U, 0U, "Fan :ON");
    oled_refresh();
}
#endif

int main(void)
{
    Dht11Data dht;
    uint16_t mq_raw;
    uint16_t demo_index;
    uint8_t startup_count;
    uint8_t oled_ok;
    uint8_t dht_ok;
    uint8_t humidity_on;
    uint8_t humidity_off;
    uint8_t mq_on;
    uint8_t mq_off;
    const char *fan_reason;
    FanState fan_state = FAN_OFF;

    SystemCoreClockUpdate();
    gpio_init_base();
    debug_led_init();
    debug_startup_blink();
    uart1_init();
    uart1_write_string("\r\nBOOT: main entered\r\n");
    delay_init();
    uart1_write_string("BOOT: delay OK\r\n");

    dht11_init();
    uart1_write_string("BOOT: DHT11 interface initialized\r\n");

    mq135_init();
    uart1_write_string("BOOT: MQ ADC interface initialized\r\n");

    fan_init();
    uart1_write_string(ENABLE_FAN_OUTPUT == 1 ? "BOOT: fan output opt-in compiled\r\n" : "BOOT: fan output disabled; pins unchanged\r\n");

    oled_ok = oled_init();
    uart1_write_string("BOOT: OLED init done\r\n");
    uart1_write_string("OLED ACK: ");
    uart1_write_string(oled_ok != 0U ? "OK" : "FAIL");
    uart1_write_string(" address=0x");
    uart1_write_string(oled_get_address() == 0x78U ? "78" : (oled_get_address() == 0x7AU ? "7A" : "??"));
    uart1_write_string("\r\n");

    uart1_write_string("BOOT: startup countdown\r\n");
    for (startup_count = 3U; startup_count > 0U; startup_count--)
    {
        oled_show_startup_countdown(startup_count);
        delay_ms(1000U);
    }

#if ENABLE_STARTUP_FAN_TEST == 1
    uart1_write_string("BOOT: fan self test opt-in\r\n");
    fan_set(FAN_ON);
    fan_state = FAN_ON;
    for (startup_count = 3U; startup_count > 0U; startup_count--)
    {
        oled_show_fan_test(startup_count);
        delay_ms(1000U);
    }
    fan_set(FAN_OFF);
    fan_state = FAN_OFF;
#else
    uart1_write_string("BOOT: startup fan test disabled\r\n");
#endif

    oled_clear();
    oled_show_string(0U, 0U, "Wardrobe Air");
    oled_show_string(2U, 0U, "System Ready");
    oled_show_string(6U, 0U, "Fan :OFF");
    oled_refresh();
    delay_ms(500U);

    while (1)
    {
        debug_led_toggle();
        mq_raw = mq135_read_average();
        /* A display-only normalized ADC index. It is not air quality, gas
         * concentration or a calibrated measurement. */
        demo_index = (uint16_t)((uint32_t)mq_raw * 100U / 4095U);
        dht_ok = (uint8_t)(dht11_read(&dht) == 0 ? 1U : 0U);
        humidity_on = 0U;
        humidity_off = 0U;
        mq_on = (uint8_t)(mq_raw >= MQ_RAW_ON_THRESHOLD ? 1U : 0U);
        mq_off = (uint8_t)(mq_raw <= MQ_RAW_OFF_THRESHOLD ? 1U : 0U);

        if (dht_ok != 0U)
        {
            humidity_on = (uint8_t)(dht.humidity >= HUMIDITY_ON_THRESHOLD ? 1U : 0U);
            humidity_off = (uint8_t)(dht.humidity <= HUMIDITY_OFF_THRESHOLD ? 1U : 0U);
        }

        if ((mq_on != 0U) || (humidity_on != 0U))
        {
            fan_set(FAN_ON);
            fan_state = FAN_ON;

            if ((mq_on != 0U) && (humidity_on != 0U))
            {
                fan_reason = "mq+humidity";
            }
            else if (mq_on != 0U)
            {
                fan_reason = "mq";
            }
            else
            {
                fan_reason = "humidity";
            }
        }
        else if ((mq_off != 0U) && ((dht_ok == 0U) || (humidity_off != 0U)))
        {
            fan_set(FAN_OFF);
            fan_state = FAN_OFF;
            fan_reason = dht_ok != 0U ? "normal" : "mq-normal";
        }
        else
        {
            fan_reason = "hold";
        }

        uart_log_status(mq_raw, demo_index, fan_state, fan_reason);
        uart_log_dht(dht_ok, &dht);
        uart1_write_string("\r\n");

        oled_show_status_screen(&dht, dht_ok, mq_raw, demo_index, fan_state, fan_reason);

        delay_ms(2000U);
    }
}
