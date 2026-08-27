/*
 * Proyecto 1 Master.c
 *
 * Created: 30/07/2026 18:27:49
 * Author : luisz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libs/lcd_hd44780.h"
#include "libs/UART.h"

/* Global Variables
**********************************
*/
volatile uint8_t Humedad = 0;
volatile uint8_t Temperatura = 0;
volatile uint8_t tick = 0;
volatile uint8_t flag_dezplazamiento = 0;

// Variables para recepción UART
char uart_rx_buffer[16];
volatile uint8_t rx_index = 0;
char val_cerradura[16] = "0"; // Valor por defecto a mostrar en (19,1)

/* Function Prototypes
**********************************
*/
void init_timer0(void);

/* ISR
**********************************
*/
ISR(TIMER1_COMPA_vect)
{
	tick++;
	flag_dezplazamiento = 0;
}

// Interrupción de Recepción UART
ISR(USART_RX_vect)
{
	char c = UDR0;
	
	if (c == '\n' || c == '\r') {
		if (rx_index > 0) {
			uart_rx_buffer[rx_index] = '\0'; // Finalizar cadena
			strncpy(val_cerradura, uart_rx_buffer, sizeof(val_cerradura) - 1);
			val_cerradura[sizeof(val_cerradura) - 1] = '\0';
			rx_index = 0;
		}
	} else {
		if (rx_index < sizeof(uart_rx_buffer) - 1) {
			uart_rx_buffer[rx_index++] = c;
		}
	}
}

/* Main
**********************************
*/
int main(void)
{
    // Inicializaciones
    lcd_init();
    initUART();    // Inicializar UART0 a 9600 baudios
    init_timer0();
    
    // Habilitar interrupciones globales
    sei();

    char buffer[16];
    
    // Configurar encabezado en la primera línea (Fila 0) - Queda estático
    lcd_set_cursor(0, 0);
    lcd_write_string("Humedad  Masa  Cerrojo  Puerta");

    while (1)
    {
        snprintf(buffer, sizeof(buffer), "0");
        
        // Humedad (1, 1)
        lcd_set_cursor(1, 1);
        lcd_write_string(buffer);
		
        // Masa (11, 1)
        lcd_set_cursor(11, 1);
        lcd_write_string(buffer);
		
        // Cerrojo / Cerradura (19, 1) -> Escribe la cadena recibida por UART
        lcd_set_cursor(19, 1);
        lcd_write_string("    "); // Limpia posibles residuos previos
        lcd_set_cursor(19, 1);
        lcd_write_string(val_cerradura);

        // Puerta (28, 1)
        lcd_set_cursor(28, 1);
        lcd_write_string(buffer);

        _delay_ms(50);
		if (flag_dezplazamiento == 0)
		{
			if (tick < 15)
			{
				lcd_command(LCD_SHIFT_DISPLAY_LEFT);
				flag_dezplazamiento = 1;
			}
			else
			{
				tick = 0;
				lcd_command(LCD_RETURN_HOME);
				flag_dezplazamiento = 1;
			}
		}

    }
}

/* Subrutinas
**********************************
*/

void init_timer0(void)
{
		// Timer1 en modo CTC, prescaler 256, 1 interrupcion cada 1 segundo
		// 16MHz / 256 = 62500 Hz -> OCR1A = 62500 - 1
		TCCR1B = (1 << WGM12) | (1 << CS12);
		TCCR1A = 0x00;
		OCR1AH = 0xF4;
		OCR1AL = 0x23;

		// Interrupciones
		TIMSK1 = (1 << OCIE1A); // Timer1 compare match A
}