/*
 * lcd_hd44780.h
 *
 * Created: 16/07/2026 15:53:22
 * Author: luisz
 */ 

#ifndef LCD_HD44780_H
#define LCD_HD44780_H

#include <avr/io.h>
#include <stdint.h>

/* ---------- Definición de pines de control ---------- */
#define LCD_CTRL_DDR    DDRC
#define LCD_CTRL_PORT   PORTC
#define LCD_RS          PC0   /* A0 */
#define LCD_E           PC1   /* A1 */

/* ---------- Definición de puertos de datos divididos ---------- */
/* D0 - D3 -> PORTD (PD4 - PD7) */
#define LCD_DATA_LOW_DDR   DDRD
#define LCD_DATA_LOW_PORT  PORTD

/* D4 - D7 -> PORTB (PB0 - PB3) */
#define LCD_DATA_HIGH_DDR  DDRB
#define LCD_DATA_HIGH_PORT PORTB

/* ---------- Comandos HD44780 ---------- */
#define LCD_CLEAR_DISPLAY   0x01
#define LCD_RETURN_HOME     0x02
#define LCD_ENTRY_MODE_SET  0x04
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_CURSOR_SHIFT    0x10
#define LCD_FUNCTION_SET    0x20
#define LCD_SET_CGRAM_ADDR  0x40
#define LCD_SET_DDRAM_ADDR  0x80

/* Flags para Entry Mode */
#define LCD_ENTRY_INCREMENT 0x02
#define LCD_ENTRY_DECREMENT 0x00
#define LCD_ENTRY_SHIFT_ON  0x01
#define LCD_ENTRY_SHIFT_OFF 0x00

/* Flags para Display Control */
#define LCD_DISPLAY_ON      0x04
#define LCD_DISPLAY_OFF     0x00
#define LCD_CURSOR_ON       0x02
#define LCD_CURSOR_OFF      0x00    
#define LCD_BLINK_ON        0x01
#define LCD_BLINK_OFF       0x00

/* Flags para Function Set */
#define LCD_8BIT_MODE       0x10
#define LCD_2LINE           0x08
#define LCD_5x8DOTS         0x00

/* ---------- Prototipos de Funciones ---------- */
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_write_char(char c);
void lcd_write_string(const char *str);
void lcd_clear(void);
void lcd_home(void);
void lcd_set_cursor(uint8_t col, uint8_t row);

/* Funciones para el control de visualización y cursor */
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_cursor_on(void);
void lcd_cursor_off(void);
void lcd_blink_on(void);
void lcd_blink_off(void);

#endif /* LCD_HD44780_H */