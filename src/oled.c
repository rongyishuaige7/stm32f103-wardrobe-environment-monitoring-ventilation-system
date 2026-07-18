#include "oled.h"
#include "app_config.h"
#include "delay.h"
#include "stm32f1xx.h"

#define OLED_WIDTH    128U
#define OLED_PAGES    8U
#define OLED_CHAR_W   6U
#define OLED_I2C_WAIT 10U

/*
 * Minimal glyph set authored for the strings emitted by this firmware.
 * Rows are stored as five vertical columns plus one blank spacer column.
 * Unsupported characters render as a blank. This deliberately replaces the
 * historical project's unlicensed full ASCII bitmap table.
 */
typedef struct
{
    char ch;
    uint8_t columns[5];
} Glyph5x7;

static const Glyph5x7 glyphs[] =
{
    {'%', {0x63U,0x13U,0x08U,0x64U,0x63U}}, {'+', {0x08U,0x08U,0x3EU,0x08U,0x08U}},
    {'-', {0x08U,0x08U,0x08U,0x08U,0x08U}}, {'.', {0x00U,0x60U,0x60U,0x00U,0x00U}},
    {':', {0x00U,0x36U,0x36U,0x00U,0x00U}}, {'0', {0x3EU,0x51U,0x49U,0x45U,0x3EU}},
    {'1', {0x00U,0x42U,0x7FU,0x40U,0x00U}}, {'2', {0x42U,0x61U,0x51U,0x49U,0x46U}},
    {'3', {0x21U,0x41U,0x45U,0x4BU,0x31U}}, {'4', {0x18U,0x14U,0x12U,0x7FU,0x10U}},
    {'5', {0x27U,0x45U,0x45U,0x45U,0x39U}}, {'6', {0x3CU,0x4AU,0x49U,0x49U,0x30U}},
    {'7', {0x01U,0x71U,0x09U,0x05U,0x03U}}, {'8', {0x36U,0x49U,0x49U,0x49U,0x36U}},
    {'9', {0x06U,0x49U,0x49U,0x29U,0x1EU}}, {'A', {0x7EU,0x11U,0x11U,0x11U,0x7EU}},
    {'C', {0x3EU,0x41U,0x41U,0x41U,0x22U}}, {'D', {0x7FU,0x41U,0x41U,0x22U,0x1CU}},
    {'F', {0x7FU,0x09U,0x09U,0x09U,0x01U}}, {'H', {0x7FU,0x08U,0x08U,0x08U,0x7FU}},
    {'I', {0x00U,0x41U,0x7FU,0x41U,0x00U}}, {'M', {0x7FU,0x02U,0x0CU,0x02U,0x7FU}},
    {'N', {0x7FU,0x04U,0x08U,0x10U,0x7FU}}, {'O', {0x3EU,0x41U,0x41U,0x41U,0x3EU}},
    {'Q', {0x3EU,0x41U,0x51U,0x21U,0x5EU}}, {'R', {0x7FU,0x09U,0x19U,0x29U,0x46U}},
    {'S', {0x46U,0x49U,0x49U,0x49U,0x31U}}, {'T', {0x01U,0x01U,0x7FU,0x01U,0x01U}},
    {'W', {0x3FU,0x40U,0x38U,0x40U,0x3FU}}, {'a', {0x20U,0x54U,0x54U,0x54U,0x78U}},
    {'d', {0x38U,0x44U,0x44U,0x48U,0x7FU}}, {'e', {0x38U,0x54U,0x54U,0x54U,0x18U}},
    {'g', {0x08U,0x14U,0x54U,0x54U,0x3CU}}, {'i', {0x00U,0x44U,0x7DU,0x40U,0x00U}},
    {'m', {0x7CU,0x04U,0x18U,0x04U,0x78U}}, {'n', {0x7CU,0x08U,0x04U,0x04U,0x78U}},
    {'o', {0x38U,0x44U,0x44U,0x44U,0x38U}}, {'p', {0x7CU,0x14U,0x14U,0x14U,0x08U}},
    {'r', {0x7CU,0x08U,0x04U,0x04U,0x08U}}, {'s', {0x48U,0x54U,0x54U,0x54U,0x20U}},
    {'t', {0x04U,0x3FU,0x44U,0x40U,0x20U}}, {'u', {0x3CU,0x40U,0x40U,0x20U,0x7CU}},
    {'y', {0x0CU,0x50U,0x50U,0x50U,0x3CU}}
};

static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t oled_address = OLED_I2C_ADDRESS;
static uint8_t oled_ready = 0U;
static uint8_t oled_ack_ok = 0U;

static void gpio_config_output_od(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;
    *cr &= ~(0xFU << shift);
    *cr |= (0x6U << shift);
}

static void gpio_config_input_pullup(GPIO_TypeDef *port, uint8_t pin)
{
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (uint32_t)(pin & 7U) * 4U;
    port->BSRR = (uint32_t)(1U << pin);
    *cr &= ~(0xFU << shift);
    *cr |= (0x8U << shift);
}

static void gpio_write_pin(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value != 0U) { port->BSRR = (uint32_t)(1U << pin); }
    else { port->BRR = (uint32_t)(1U << pin); }
}

static uint8_t gpio_read_pin(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR & (1U << pin)) ? 1U : 0U;
}

static void oled_scl_low(void) { gpio_config_output_od(OLED_SCL_PORT, OLED_SCL_PIN); gpio_write_pin(OLED_SCL_PORT, OLED_SCL_PIN, 0U); }
static void oled_scl_release(void) { gpio_config_input_pullup(OLED_SCL_PORT, OLED_SCL_PIN); }
static void oled_sda_low(void) { gpio_config_output_od(OLED_SDA_PORT, OLED_SDA_PIN); gpio_write_pin(OLED_SDA_PORT, OLED_SDA_PIN, 0U); }
static void oled_sda_release(void) { gpio_config_input_pullup(OLED_SDA_PORT, OLED_SDA_PIN); }

static void oled_i2c_start(void)
{
    oled_sda_release(); oled_scl_release(); delay_us(OLED_I2C_WAIT);
    oled_sda_low(); delay_us(OLED_I2C_WAIT); oled_scl_low();
}

static void oled_i2c_stop(void)
{
    oled_sda_low(); oled_scl_release(); delay_us(OLED_I2C_WAIT);
    oled_sda_release(); delay_us(OLED_I2C_WAIT);
}

static uint8_t oled_i2c_write_byte(uint8_t byte)
{
    uint8_t i;
    uint8_t ack;
    for (i = 0U; i < 8U; i++)
    {
        if ((byte & 0x80U) != 0U) { oled_sda_release(); } else { oled_sda_low(); }
        oled_scl_release(); delay_us(OLED_I2C_WAIT); oled_scl_low(); delay_us(OLED_I2C_WAIT);
        byte <<= 1U;
    }
    oled_sda_release(); oled_scl_release(); delay_us(OLED_I2C_WAIT);
    ack = (uint8_t)(gpio_read_pin(OLED_SDA_PORT, OLED_SDA_PIN) == 0U);
    oled_scl_low(); delay_us(OLED_I2C_WAIT);
    return ack;
}

static uint8_t oled_probe(void)
{
    uint8_t ack;
    oled_i2c_start(); ack = oled_i2c_write_byte(oled_address); oled_i2c_stop();
    return ack;
}

static void oled_write_cmd(uint8_t cmd)
{
    oled_i2c_start(); (void)oled_i2c_write_byte(oled_address);
    (void)oled_i2c_write_byte(0x00U); (void)oled_i2c_write_byte(cmd); oled_i2c_stop();
}

static void oled_write_data(uint8_t data)
{
    oled_i2c_start(); (void)oled_i2c_write_byte(oled_address);
    (void)oled_i2c_write_byte(0x40U); (void)oled_i2c_write_byte(data); oled_i2c_stop();
}

static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_write_cmd((uint8_t)(0xB0U + page));
    oled_write_cmd((uint8_t)(0x10U | ((col >> 4U) & 0x0FU)));
    oled_write_cmd((uint8_t)(col & 0x0FU));
}

static const uint8_t *glyph_columns(char ch)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)(sizeof(glyphs) / sizeof(glyphs[0])); i++)
    {
        if (glyphs[i].ch == ch) { return glyphs[i].columns; }
    }
    return 0;
}

static void oled_draw_char(uint8_t row, uint8_t col, char ch)
{
    uint8_t i;
    uint16_t index;
    const uint8_t *columns;
    if ((row >= OLED_PAGES) || (col >= (OLED_WIDTH / OLED_CHAR_W))) { return; }
    index = (uint16_t)row * OLED_WIDTH + (uint16_t)col * OLED_CHAR_W;
    columns = glyph_columns(ch);
    for (i = 0U; i < 5U; i++) { oled_buffer[index + i] = columns != 0 ? columns[i] : 0U; }
    oled_buffer[index + 5U] = 0U;
}

uint8_t oled_init(void)
{
    uint16_t i;
    uint8_t addr_index;
    uint8_t address_list[2];
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    gpio_config_input_pullup(OLED_SCL_PORT, OLED_SCL_PIN);
    gpio_config_input_pullup(OLED_SDA_PORT, OLED_SDA_PIN);
    address_list[0] = OLED_I2C_ADDRESS;
    address_list[1] = (OLED_I2C_ADDRESS == 0x78U) ? 0x7AU : 0x78U;
    delay_ms(100U);
    oled_ack_ok = 0U;
    for (addr_index = 0U; addr_index < 2U; addr_index++)
    {
        oled_address = address_list[addr_index];
        if (oled_probe() != 0U) { oled_ack_ok = 1U; break; }
    }
    if (oled_ack_ok == 0U) { oled_address = OLED_I2C_ADDRESS; }
    oled_ready = 1U;
    oled_write_cmd(0xAEU); oled_write_cmd(0xD5U); oled_write_cmd(0x80U);
    oled_write_cmd(0xA8U); oled_write_cmd(0x3FU); oled_write_cmd(0xD3U);
    oled_write_cmd(0x00U); oled_write_cmd(0x40U); oled_write_cmd(0xA1U);
    oled_write_cmd(0xC8U); oled_write_cmd(0xDAU); oled_write_cmd(0x12U);
    oled_write_cmd(0x81U); oled_write_cmd(0xCFU); oled_write_cmd(0xD9U);
    oled_write_cmd(0xF1U); oled_write_cmd(0xDBU); oled_write_cmd(0x40U);
    oled_write_cmd(0xA4U); oled_write_cmd(0xA6U); oled_write_cmd(0x8DU);
    oled_write_cmd(0x14U); oled_write_cmd(0xAFU);
    for (i = 0U; i < (OLED_WIDTH * OLED_PAGES); i++) { oled_buffer[i] = 0U; }
    oled_refresh();
    return oled_ack_ok;
}

uint8_t oled_get_address(void) { return oled_address; }
void oled_clear(void) { oled_fill(0U); }
void oled_fill(uint8_t pattern)
{
    uint16_t i;
    for (i = 0U; i < (OLED_WIDTH * OLED_PAGES); i++) { oled_buffer[i] = pattern; }
}
void oled_show_string(uint8_t row, uint8_t col, const char *str)
{
    while ((*str != '\0') && (col < (OLED_WIDTH / OLED_CHAR_W))) { oled_draw_char(row, col++, *str++); }
}
void oled_refresh(void)
{
    uint8_t page;
    uint8_t col;
    if (oled_ready == 0U) { return; }
    for (page = 0U; page < OLED_PAGES; page++)
    {
        oled_set_pos(page, 0U);
        for (col = 0U; col < OLED_WIDTH; col++) { oled_write_data(oled_buffer[page * OLED_WIDTH + col]); }
    }
}
