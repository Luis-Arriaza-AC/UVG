/*
 * Digi 2 Lab 2.c
 *
 * Created: 16/07/2026 15:52:48
 * Author : luisz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#include "libs/lcd_hd44780.h"
#include "libs/UART.h"

/* Global Variables
**********************************
*/
volatile uint8_t contador_s3 = 0;
volatile uint8_t flag_send_uart = 0;

/* Function Prototypes
**********************************
*/
void adc_init(void);
uint16_t adc_read(uint8_t channel);

/* ISR Recepción UART
**********************************
*/
ISR(USART_RX_vect)
{
    char rx = UDR0;
    
    if (rx == 'D' || rx == 'd')
    {
        flag_send_uart = 1;
    }
    else if (rx == '+')
    {
        contador_s3++;
    }
    else if (rx == '-')
    {
        contador_s3--;
    }
}

/* Main
**********************************
*/
int main(void)
{
    // Inicializaciones
    lcd_init();
    adc_init();
    initUART();
    
    // Mensaje de inicio en consola serial
    writeString("Inserte D para mostrar datos, envie + o - para manipular S3.\r\n");
    
    // Habilitar interrupciones globales
    sei();

    char buffer[16];
    
    // Configurar encabezado en la primera línea (Fila 0) - Queda estático
    lcd_set_cursor(0, 0);
    lcd_write_string("   S1   S2   S3   ");

    while (1)
    {
        /* --- Parte 1: Lectura S1 (ADC6) y cálculo de voltaje (0.00v) --- */
        uint16_t adc_s1 = adc_read(6);
        
        // Conversión a voltaje escalada a 0.00v - 5.00v
        float volt = ((float)adc_s1 * 5.0f) / 1023.0f;
        uint8_t enterosh = (uint8_t)volt;
        uint8_t decimales = (uint8_t)((volt - enterosh) * 100.0f);

        /* --- Parte 2: Lectura S2 (ADC5) --- */
        uint16_t adc_s2 = adc_read(5);

        /* --- Parte 3: Envío por UART únicamente cuando se recibe 'D' --- */
        if (flag_send_uart)
        {
            snprintf(buffer, sizeof(buffer), "S1:%u.%02uv S2:%u \r\n", enterosh, decimales, adc_s2);
            writeString(buffer);
            flag_send_uart = 0; // Limpiar la bandera hasta el próximo comando 'D'
        }
        
        // S1 en formato 0.00v bajo la etiqueta S1 (Columna 1)
        lcd_set_cursor(1, 1);
        snprintf(buffer, sizeof(buffer), "%u.%02uv", enterosh, decimales);
        lcd_write_string(buffer);

        // S2 en formato 0 - 1023 (Columna 7)
        lcd_set_cursor(7, 1);
        snprintf(buffer, sizeof(buffer), "%4u", adc_s2);
        lcd_write_string(buffer);

        // S3 en formato 0 - 255 (Columna 13)
        lcd_set_cursor(12, 1);
        snprintf(buffer, sizeof(buffer), "%3u", contador_s3);
        lcd_write_string(buffer);

        _delay_ms(50);
    }
}

/* Subrutinas ADC
**********************************
*/
void adc_init(void)
{
    // Vref = AVCC (5V), alineación a la derecha
    ADMUX = (1 << REFS0);
    
    // ADC Enable, Prescaler = 128 (16MHz / 128 = 125kHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    
    return ADC;
}