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
volatile uint8_t seg_high = 0;
volatile uint8_t seg_low = 0;
volatile uint8_t valor = 0;

const uint8_t seg7[16] = {
	0x3F, // 0  0011 1111
	0x06, // 1  0000 0110
	0x5B, // 2  0101 1011
	0x4F, // 3  0100 1111
	0x66, // 4  0110 0110
	0x6D, // 5  0110 1101
	0x7D, // 6  0111 1101
	0x07, // 7  0000 0111
	0x7F, // 8  0111 1111
	0x6F, // 9  0110 1111
	0x77, // A  0111 0111
	0x7C, // B  0111 1100
	0x39, // C  0011 1001
	0x5E, // D  0101 1110
	0x79, // E  0111 1001
	0x71, // F  0111 0001
};      
/****************************************/
// Function prototypes
void setup();
void multiplex(uint8_t disp_iz, uint8_t disp_der);
uint16_t leer_adc();
void led_alarma(uint8_t displays);
/****************************************/
// Main Function
int main(void)
{
	setup();

	while (1)
	{
		uint16_t valor = leer_adc();
		uint8_t conversion = (uint8_t)((valor*256UL)/1024);
		uint8_t seg_high = (conversion>>4);
		uint8_t seg_low = (conversion) & (0x0F);
		multiplex(seg_high, seg_low);
		led_alarma(conversion);
	}
}

/****************************************/
// NON-Interrupt subroutines
void setup()
{
	// Puerto D, PC2, PB0 como salida
	DDRD  = 0xFF;
	PORTD = 0x00;
	
	DDRC  |=  (1 << PC2);
	PORTC &= ~(1 << PC2);
	
	DDRB  = (1<<PB0);
	PORTB = ~(1 << PB0);

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

    // ADC PC6,
    ADMUX  = (1 << REFS0) | (1 << MUX2) | (1 << MUX1);
    // Prescaler 128
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Habilitar interrupciones globales
	sei();
}
//Multiplexado
void multiplex(uint8_t disp_iz, uint8_t disp_der)
{
	//Display iz
	PORTC &= ~((1<<PC1)|(1<<PC2));
	PORTD = seg7[disp_iz];
	PORTC |= (1<<PC0);
	_delay_ms(3);
	
	//Display der
	PORTC &= ~((1<<PC0)|(1<<PC2));
	PORTD = seg7[disp_der];
	PORTC |= (1<<PC1);
	_delay_ms(3);
	
	//Leds
	PORTC &= ~((1<<PC0)|(1<<PC1));
	PORTD = contador;
	PORTC |= (1<<PC2);
	_delay_ms(3);
	
}

//Lectura adc del potenciometro
uint16_t leer_adc()
{
	ADCSRA |= (1 << ADSC);           // Iniciar ADC
	while (ADCSRA & (1 << ADSC));    // Esperar termine de leer
	return ADC;
}

void led_alarma(uint8_t displays)
{
	if(contador < displays)
	{
		PORTB |= (1<<PB0);
	}else
	{
		PORTB &= ~(1<<PB0);
	}
}
/****************************************/
// Interrupt routines

//Puerto b, verifica, resta
ISR(PCINT0_vect)
{
    uint8_t puertob = PINB;
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

//Puerto c, verifica, suma
ISR(PCINT1_vect)
{
	uint8_t puertoc = PINC;
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