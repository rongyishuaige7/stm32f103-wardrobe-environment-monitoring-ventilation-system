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
    uint8_t rows[5];
} Glyph4x5;

/* Four-bit rows, independently redrawn for this publication. Keeping only the
 * characters used by the UI makes the provenance and public scope explicit. */
static const Glyph4x5 glyphs[] =
{
    {'%', {0x9U,0x2U,0x4U,0x8U,0x9U}}, {'+', {0x0U,0x2U,0x7U,0x2U,0x0U}},
    {'-', {0x0U,0x0U,0xFU,0x0U,0x0U}}, {'.', {0x0U,0x0U,0x0U,0x0U,0x2U}},
    {':', {0x0U,0x2U,0x0U,0x2U,0x0U}}, {'0', {0x6U,0x9U,0x9U,0x9U,0x6U}},
    {'1', {0x2U,0x6U,0x2U,0x2U,0x7U}}, {'2', {0x6U,0x9U,0x2U,0x4U,0xFU}},
    {'3', {0xEU,0x1U,0x6U,0x1U,0xEU}}, {'4', {0x9U,0x9U,0xFU,0x1U,0x1U}},
    {'5', {0xFU,0x8U,0xEU,0x1U,0xEU}}, {'6', {0x6U,0x8U,0xEU,0x9U,0x6U}},
    {'7', {0xFU,0x1U,0x2U,0x4U,0x4U}}, {'8', {0x6U,0x9U,0x6U,0x9U,0x6U}},
    {'9', {0x6U,0x9U,0x7U,0x1U,0x6U}}, {'A', {0x6U,0x9U,0xFU,0x9U,0x9U}},
    {'C', {0x7U,0x8U,0x8U,0x8U,0x7U}}, {'D', {0xEU,0x9U,0x9U,0x9U,0xEU}},
    {'F', {0xFU,0x8U,0xEU,0x8U,0x8U}}, {'H', {0x9U,0x9U,0xFU,0x9U,0x9U}},
    {'I', {0x7U,0x2U,0x2U,0x2U,0x7U}}, {'L', {0x8U,0x8U,0x8U,0x8U,0xFU}},
    {'M', {0x9U,0xFU,0xFU,0x9U,0x9U}}, {'N', {0x9U,0xDU,0xBU,0x9U,0x9U}},
    {'O', {0x6U,0x9U,0x9U,0x9U,0x6U}}, {'Q', {0x6U,0x9U,0x9U,0xBU,0x7U}},
    {'R', {0xEU,0x9U,0xEU,0xAU,0x9U}}, {'S', {0x7U,0x8U,0x6U,0x1U,0xEU}},
    {'T', {0xFU,0x2U,0x2U,0x2U,0x2U}}, {'W', {0x9U,0x9U,0x9U,0xFU,0x6U}},
    {'a', {0x0U,0x6U,0x1U,0x7U,0x7U}}, {'b', {0x8U,0x8U,0xEU,0x9U,0xEU}},
    {'d', {0x1U,0x1U,0x7U,0x9U,0x7U}}, {'e', {0x0U,0x6U,0xFU,0x8U,0x7U}},
    {'f', {0x3U,0x4U,0xEU,0x4U,0x4U}}, {'g', {0x0U,0x7U,0x9U,0x7U,0x1U}},
    {'i', {0x2U,0x0U,0x6U,0x2U,0x7U}}, {'l', {0x6U,0x2U,0x2U,0x2U,0x7U}},
    {'m', {0x0U,0xAU,0xFU,0x9U,0x9U}}, {'n', {0x0U,0xEU,0x9U,0x9U,0x9U}},
    {'o', {0x0U,0x6U,0x9U,0x9U,0x6U}}, {'p', {0x0U,0xEU,0x9U,0xEU,0x8U}},
    {'r', {0x0U,0xBU,0xCU,0x8U,0x8U}}, {'s', {0x0U,0x7U,0xCU,0x3U,0xEU}},
    {'t', {0x4U,0xEU,0x4U,0x4U,0x3U}}, {'u', {0x0U,0x9U,0x9U,0x9U,0x7U}},
    {'x', {0x0U,0x9U,0x6U,0x6U,0x9U}}, {'y', {0x0U,0x9U,0x7U,0x1U,0x6U}}
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

static const uint8_t *glyph_rows(char ch)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)(sizeof(glyphs) / sizeof(glyphs[0])); i++)
    {
        if (glyphs[i].ch == ch) { return glyphs[i].rows; }
    }
    return 0;
}

static void oled_draw_char(uint8_t row, uint8_t col, char ch)
{
    uint8_t column;
    uint8_t pixel_row;
    uint16_t index;
    const uint8_t *rows;
    if ((row >= OLED_PAGES) || (col >= (OLED_WIDTH / OLED_CHAR_W))) { return; }
    index = (uint16_t)row * OLED_WIDTH + (uint16_t)col * OLED_CHAR_W;
    rows = glyph_rows(ch);
    for (column = 0U; column < 4U; column++)
    {
        uint8_t bits = 0U;
        if (rows != 0)
        {
            for (pixel_row = 0U; pixel_row < 5U; pixel_row++)
            {
                if ((rows[pixel_row] & (uint8_t)(1U << (3U - column))) != 0U)
                {
                    bits |= (uint8_t)(1U << pixel_row);
                }
            }
        }
        oled_buffer[index + column] = bits;
    }
    oled_buffer[index + 4U] = 0U;
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
