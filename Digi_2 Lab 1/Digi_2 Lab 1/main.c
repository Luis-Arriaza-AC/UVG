/*
 * Digi_2 Lab 1.c
 *
 * Juego de carrera binaria
 * Created: 09/07/2026 17:27:46
 * Author : luisz
 */ 
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "libs/_7seg_disp.h"

/*Function Prototypes
**********************************
*/
void setup(void);
void display_leds(void);
void reset_juego(void);

/*Variables globales
**********************************
*/
volatile uint8_t contador1   = 0;   // contador binario jugador 1
volatile uint8_t contador2   = 0;   // contador binario jugador 2
volatile uint8_t num_disp    = 5;   // valor cuenta regresiva
volatile uint8_t estado      = 0;   // 0=iddle, 1=cuenta regresiva, 2=juego, 3=ganador
volatile uint8_t ganador     = 0;
volatile uint8_t flag_p1     = 0;   // bandera boton jugador1 presionado
volatile uint8_t flag_p2     = 0;   // bandera boton jugador2 presionado
volatile uint8_t tick        = 0;   // bandera de 1 seg generada por Timer1
volatile uint8_t blink_on    = 0;   // parapdeo en idle
volatile uint8_t p1_soltado    = 1; //Bandera de flanco jugador 1
volatile uint8_t p2_soltado    = 1;	//Bandera de flanco jugador 2
volatile uint8_t reset_soltado = 1;

/*Main
**********************************
*/
int main(void)
{
	cli();
	setup();
	sei();

	while (1)
	{
		if (tick)
		{
			tick = 0;

			if (estado == 0) //parpadeo del "0" en idle
			{
				blink_on ^= 1;
			}
			else if (estado == 1) // cuenta regresiva
			{
				if (num_disp > 0)
				{
					num_disp--;
				}
				else
				{
					estado   = 2;   // modo juego
					num_disp = 5;   // reset de variable
				}
			}
		}

		if (estado == 2) //Lógica del juego
		{
			if (flag_p1)
			{
				flag_p1 = 0;
				contador1 = (contador1 == 0) ? 1 : (contador1 << 1);
				if (contador1 == 0x08) //Verificación de ganador
				{
					ganador   = 1;
					contador1 = 0x0F; // enciende leds del ganador
					contador2 = 0x00; // apaga 4 leds del perdedor
					estado    = 3;
				}
			}
			if (flag_p2)
			{
				flag_p2 = 0;
				contador2 = (contador2 == 0) ? 1 : (contador2 << 1);
				if (contador2 == 0x08) //Verificación de ganador
				{
					ganador   = 2;
					contador2 = 0x0F; // enciende leds del ganador
					contador1 = 0x00; // apaga 4 leds del perdedor
					estado    = 3;
				}
			}
		}

		switch (estado) //Lógica Display 7 seg
		{
			case 0: // iddle
				if (blink_on) disp_mostrar(0);
				else          disp_apagar();
				break;

			case 1: // cuenta regresiva
				disp_mostrar(num_disp);
				break;

			case 2: //disp apagado
				disp_apagar();
				break;

			case 3: // muestra al ganador (1 o 2)
				disp_mostrar(ganador);
				break;
		}

		// ---- Refresco de LEDs multiplexados (PB0-PB3) ----
		display_leds();
	}
}

/*Non-interrupt subroutine
**********************************
*/
void setup(void)
{
	DDRD  = 0xFF; // Puerto D completo salida display
	PORTD = 0x00;

	DDRB  = 0x3F; // PB0-PB3 LEDs, PB4-PB5 bases de transistores
	PORTB = 0x00;

	DDRC  = 0x00; // A0-A2 como entradas
	PORTC = 0x07; // Pull-up interno en A0, A1, A2

	// Timer1 en modo CTC, prescaler 256, 1 interrupcion cada 1 segundo
	// 16MHz / 256 = 62500 Hz -> OCR1A = 62500 - 1
	TCCR1B = (1 << WGM12) | (1 << CS12);
	TCCR1A = 0x00;
	OCR1AH = 0xF4;
	OCR1AL = 0x23;

	// Interrupciones
	TIMSK1 = (1 << OCIE1A);                                  // Timer1 compare match A
	PCICR  = (1 << PCIE1);                                   // PCINT Puerto C
	PCMSK1 = (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10); // A0, A1, A2
}

void reset_juego(void)
{
	contador1 = 0;
	contador2 = 0;
	ganador   = 0;
	flag_p1   = 0;
	flag_p2   = 0;
	num_disp  = 5;
}

void display_leds(void)
{
	// Grupo 1 -> contador jugador 1 (transistor en PB4)
	PORTB = (contador1 & 0x0F) | (1 << 4);
	_delay_ms(2);
	PORTB = 0x00;

	// Grupo 2 -> contador jugador 2 (transistor en PB5)
	PORTB = (contador2 & 0x0F) | (1 << 5);
	_delay_ms(2);
	PORTB = 0x00;
}

/*Interrupt subroutine
**********************************
*/
ISR(TIMER1_COMPA_vect)
{
	tick = 1; //bandera del timer
}

ISR(PCINT1_vect)
{
	uint8_t boton = PINC & 0x07;

	// Botón de reset
	if (!(boton & (1 << PC2)))
	{
		if (reset_soltado && (estado == 0 || estado == 3))
		{
			reset_juego();
			TCNT1H = 0x00;
			TCNT1L = 0x00;
			estado = 1; // inicia cuenta regresiva
		}
		reset_soltado = 0;
	}
	else
	{
		reset_soltado = 1;
	}

	// Boton P1
	if (!(boton & (1 << PC1)))
	{
		if (p1_soltado && estado == 2) flag_p1 = 1;
		p1_soltado = 0;
	}
	else
	{
		p1_soltado = 1;
	}

	//Boton P2
	if (!(boton & (1 << PC0)))
	{
		if (p2_soltado && estado == 2) flag_p2 = 1;
		p2_soltado = 0;
	}
	else
	{
		p2_soltado = 1;
	}
}