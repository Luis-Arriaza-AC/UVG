/*
 * Lab4.c
 *
 * Created: 10/4/26
 * Author: Luis Arriaza
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

volatile uint8_t contador = 0;
#define DEBOUNCE_MS 25      
/****************************************/
// Function prototypes
void setup();
void actualizar_leds();
/****************************************/
// Main Function
int main(void)
{
	setup();
	while (1)
	{
		actualizar_leds();
	}
}

/****************************************/
// NON-Interrupt subroutines
void setup()
{
	// Puerto D y PC2 como salida
	DDRD  = 0xFF;
	PORTD = 0x00;
	
	DDRC  |=  (1 << PC2);
	PORTC &= ~(1 << PC2);

	//PB4 y PC4 como entradas con pull-up interno
	DDRB  &= ~(1 << PB4);
	PORTB |=  (1 << PB4);

    DDRC  &= ~(1 << PC4);
    PORTC |=  (1 << PC4);

	// Habilitar PCINT port B y C
	PCICR  |= (1 << PCIE0)|(1 << PCIE1);

	// Habilitar Pines PCINT
	PCMSK0 |= (1 << PCINT4);
	PCMSK1 |= (1 << PCINT12);

	// Habilitar interrupciones globales
	sei();

	// Encender transistor
	PORTC |= (1 << PC2);
}

void actualizar_leds()
{
	PORTD = contador;
}

/****************************************/
// Interrupt routines

//Puerto b, debounce, verifica, resta
ISR(PCINT0_vect)
{
    uint8_t puertob = PINB;
	_delay_ms(DEBOUNCE_MS);
	uint8_t puertob_debounce = PINB;
	
	if (puertob == puertob_debounce)
	{
		puertob &= (1<<PB4);
		if (puertob == 0)
		{
			contador--;
		}
	}
}

//Puerto c, debounce, verifica, suma
ISR(PCINT1_vect)
{
	uint8_t puertoc = PINC;
	_delay_ms(DEBOUNCE_MS);
	uint8_t puertoc_debounce = PINC;
	
	if (puertoc == puertoc_debounce)
	{
		puertoc &= (1<<PC4);
		if (puertoc == 0)
		{
			contador++;
		}
	}
}