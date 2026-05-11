/*
 * Lab6
 *
 * Created: 24/04/2026
 * Author: luisz
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "libreria/UART.h"

/****************************************/
// Function prototypes
void setup(void);
uint16_t leerADC(void);
void leds(uint8_t dato);

/****************************************/
// Variables globales

volatile uint8_t datoUSB = 0;

/****************************************/
// Main Function

int main(void)
{
    cli();
    setup();
    initUART();
	writeString("HOLA MUNDO ");
    sei();
	
    while (1)
    {
        leds(datoUSB);
    }
}

/****************************************/
// NON-Interrupt subroutines

void setup(void)
{
	
	DDRB  = 0x3F; // PB 0-5 salida
	PORTB = 0x00;
	//ADC
    DIDR0 |= (1<<ADC0D);
    ADMUX  = (1<<REFS0); // AVcc, ADC0
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // prescaler 128
}

uint16_t leerADC(void)
{
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
    return ADC;
}

void leds(uint8_t ascii)
{
    uint8_t bajo = ascii & 0x0F;
    uint8_t alto = (ascii >> 4); 

//Mitad baja
    PORTB = (1<<PB5) | bajo;
    _delay_ms(2);

//Mitad alta
    PORTB = (1<<PB4) | alto;
    _delay_ms(2);
}

/****************************************/
// Interrupt routines

ISR(USART_RX_vect)
{
    datoUSB = UDR0;      // Guardar dato UART
    writeChar(datoUSB);  // Eco hacia terminal
}