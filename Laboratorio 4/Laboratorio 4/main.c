x/*
 * Lab 6.c
 *
 * Created: 24/04/2026 15:09:43
 * Author : luisz
 */ 

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

	DDRB  = 0xF; // PB0–PB3 salida
	PORTB = 0x00;
	DDRD  |= 0x0C; //PD2-PD3 salida transistores
	

	setup();
	initUART();

	sei();

	while (1)
	{
		// ADC -> UART
		uint16_t valor = leerADC();
		char caracter = (char)((valor * 94UL) / 1023 + 0x20);
		writeChar(caracter);

		// Mostrar recibido (por el multiplexado)
		leds(datoUSB);
	}
}

/****************************************/
// NON-Interrupt subroutines

void setup(void)
{
	//ADC
	DIDR0 |= (1<<ADC0D);
	ADMUX  = (1 << REFS0) | (1 << MUX2) | (1 << MUX1); // AVcc, ADC6
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // prescaler 128
}

uint16_t leerADC(void)
{
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC));
	return ADC;
}

void leds(uint8_t dato)
{
	uint8_t bajo = dato & 0x0F;
	uint8_t alto = (dato >> 4) & 0x0F;

	// Nibble bajo
	PORTD &= ~(1<<0x08);
	PORTB = bajo;
	PORTD |= 0x04;
	_delay_ms(2);

	// Nibble alto
	PORTD &= ~(1<<0x04);
	PORTB = alto;
	PORTD |= 0x08;
	_delay_ms(2);
}

/****************************************/
// Interrupt routines

ISR(USART_RX_vect)
{
	datoUSB = UDR0;      // Guardar dato recibido
	writeChar(datoUSB);  // Eco hacia terminal
}

