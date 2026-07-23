/*
 * lcd_hd44780.c
 *
 * Created: 16/07/2026 15:53:11
 * Author: luisz
 */ 
#include "lcd_hd44780.h"
#include <util/delay.h>

/* Variable interna para guardar el estado del display (ON/OFF, Cursor, Blink) */
static uint8_t display_control_state = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;

static void lcd_send_byte(uint8_t byte, uint8_t rs)
{
    /* 
     * D0 - D3 -> Nibble mayor de PORTD (PD4..PD7)
     * Limpiamos PD4..PD7 y colocamos (byte & 0x0F) desplazado 4 bits a la izquierda
     */
    LCD_DATA_LOW_PORT = (LCD_DATA_LOW_PORT & 0x0F) | ((byte & 0x0F) << 4);

    /* 
     * D4 - D7 -> Nibble menor de PORTB (PB0..PB3)
     * Limpiamos PB0..PB3 y colocamos la parte alta del byte (byte >> 4)
     */
    LCD_DATA_HIGH_PORT = (LCD_DATA_HIGH_PORT & 0xF0) | ((byte >> 4) & 0x0F);

    /* Selecciona el registro (RS) */
    if (rs)
        LCD_CTRL_PORT |= (1 << LCD_RS);
    else
        LCD_CTRL_PORT &= ~(1 << LCD_RS);

    /* Tiempo de setup de RS/datos (tAS >= 40 ns) */
    _delay_us(1);

    /* Pulso de Enable */
    LCD_CTRL_PORT |= (1 << LCD_E);
    _delay_us(1);
    LCD_CTRL_PORT &= ~(1 << LCD_E);

    /* Tiempo de hold después de bajar E (tH >= 10 ns) */
    _delay_us(1);
}

void lcd_command(uint8_t cmd)
{
    lcd_send_byte(cmd, 0);

    if (cmd == LCD_CLEAR_DISPLAY || cmd == LCD_RETURN_HOME)
        _delay_ms(2);      /* Requerimiento datasheet ~1.52 ms */
    else
        _delay_us(50);     /* Requerimiento datasheet ~37 us */
}

void lcd_write_char(char c)
{
    lcd_send_byte((uint8_t)c, 1);
    _delay_us(50);
}

void lcd_write_string(const char *str)
{
    while (*str != '\0')
    {
        lcd_write_char(*str);
        str++;
    }
}

void lcd_init(void)
{
    /* Configura PD4..PD7 como salidas */
    LCD_DATA_LOW_DDR |= 0xF0;

    /* Configura PB0..PB3 como salidas */
    LCD_DATA_HIGH_DDR |= 0x0F;

    /* Configura RS (PC0) y E (PC1) como salidas */
    LCD_CTRL_DDR |= (1 << LCD_RS) | (1 << LCD_E);
    LCD_CTRL_PORT &= ~((1 << LCD_RS) | (1 << LCD_E));

    _delay_ms(40);

    /* Secuencia de inicialización en 8 bits según datasheet */
    lcd_send_byte(0x30, 0);
    _delay_ms(5);

    lcd_send_byte(0x30, 0);
    _delay_us(150);

    lcd_send_byte(0x30, 0);
    _delay_us(150);

    /* Configuración de función: 8 bits, 2 líneas, fuente 5x8 */
    lcd_command(LCD_FUNCTION_SET | LCD_8BIT_MODE | LCD_2LINE | LCD_5x8DOTS);

    /* Estado inicial: Display encendido, cursor off, blink off */
    display_control_state = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);

    /* Limpiar pantalla */
    lcd_clear();

    /* Incremento del cursor automático, sin desplazar pantalla */
    lcd_command(LCD_ENTRY_MODE_SET | LCD_ENTRY_INCREMENT | LCD_ENTRY_SHIFT_OFF);
}

void lcd_clear(void)
{
    lcd_command(LCD_CLEAR_DISPLAY);
}

void lcd_home(void)
{
    lcd_command(LCD_RETURN_HOME);
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

    if (row > 3) row = 3;

    lcd_command(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

/* ---------- Control de Display y Cursor ---------- */

void lcd_display_on(void)
{
    display_control_state |= LCD_DISPLAY_ON;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}

void lcd_display_off(void)
{
    display_control_state &= ~LCD_DISPLAY_ON;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}

void lcd_cursor_on(void)
{
    display_control_state |= LCD_CURSOR_ON;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}

void lcd_cursor_off(void)
{
    display_control_state &= ~LCD_CURSOR_ON;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}

void lcd_blink_on(void)
{
    display_control_state |= LCD_BLINK_ON;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}

void lcd_blink_off(void)
{
    display_control_state &= ~LCD_BLINK_OFF;
    lcd_command(LCD_DISPLAY_CONTROL | display_control_state);
}