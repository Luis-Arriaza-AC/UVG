/*
* Timer.asm
*
* Creado: 13/2/26
* Autor : Luis
* Descripción: Timer
*/
/****************************************/
// Encabezado (Definición de Registros, Variables y Constantes)
.include "M328PDEF.inc"     // Include definitions specific to ATMega328P
.dseg
.org    SRAM_START
//variable_name:     .byte   1   // Memory alocation for variable_name:     .byte   (byte size)

.cseg
.org 0x0000
RJMP SETUP

.org 0x0006 // Vector PCINT0 Puerto B
RJMP PCINT0_ISR

.org 0x0008 // Vector PCINT1 Puerto C
RJMP PCINT1_ISR


 /****************************************/
// Configuración de la pila
LDI     R16, LOW(RAMEND)
OUT     SPL, R16
LDI     R16, HIGH(RAMEND)
OUT     SPH, R16
/****************************************/
// Configuracion MCU
SETUP:
	//clock
	LDI		R16, (1<<CLKPCE) //Habilita prescaler clock
	STS		CLKPR, R16
	LDI		R16, (1<<CLKPS1)|(1<<CLKPS0) //Dividir entre 8
	STS		CLKPR, R16

	//Puertos
	LDI		R16,	0xFF //Todo puerto D salida para display
	OUT		DDRD,	R16
	LDI		R16,	0x03 //Pin 0-1 puerto C como Salida
	OUT		DDRC,	R16
	LDI		R16,	0x0F //Pin 0-3 puerto B salida leds contador botones
	OUT		DDRB,	R16
	LDI		R16,	0x00 //Apagar display
	OUT		PORTD,	R16
	LDI		R16,	0x10 //Encender pullup portB pin 4
	OUT		PORTB,	R16
	LDI		R16,	0x04 //Encender pullup portC pin 2
	OUT		PORTC,	R16

	//Interrupciones
	LDI		R16, (1<<PCIE0)|(1<<PCIE1) // Habilitar PCINT en Puerto B y Puerto C
	STS		PCICR, R16
	LDI		R16, (1<<PCINT4) //Pin 4 de puerto B
	STS		PCMSK0, R16
	LDI		R16, (1<<PCINT10) //Pin 3 de puerto C
	STS		PCMSK1, R16
	SEI

	//Registros de contador
	LDI		R22,	0x00 //Registro contador display
	LDI		R23,	0x00 //registro contador overflows timer
	LDI		R18,	0x00 //Registro de contador leds
	LDI		R17,	0x10 //Constante a guardar para maneter pull up en port B
	LDI		R19,	0x04 //Constante para maneter pull up en port C

	//Timer
	LDI		R16,	0x00 //Timer0 modo normal
	OUT		TCCR0A,	R16
	LDI		R16,	(1<<CS02)|(1<<CS00) //Prescaler timer 1024
	OUT		TCCR0B, R16
	LDI		R16, 61 //Precargar 61 a TCNT0
	OUT		TCNT0, R16

/****************************************/
// Loop Infinito
MAIN_LOOP:
	CALL	Contador
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines

Contador:
	ANDI	R18, 0x0F
	OR		R18, R17
	OUT		PORTB, R18
	RET

Retorno:
	RET

DISPLAY:
	CPI		R22, 0x00
	BRBS	1, Cero
	CPI		R22, 0x01
	BRBS	1, Uno
	CPI		R22, 0x02
	BRBS	1, Dos
	CPI		R22, 0x03
	BRBS	1, Tres
	CPI		R22, 0x04
	BRBS	1, Cuatro
	CPI		R22, 0x05
	BRBS	1, Cinco
	CPI		R22, 0x06
	BRBS	1, Seis
	CPI		R22, 0x07
	BRBS	1, Siete
	CPI		R22, 0x08
	BRBS	1, Ocho
	CPI		R22, 0x09
	BRBS	1, Nueve
	CPI		R22, 0x0A
	BRBS	1, Diez
	CPI		R22, 0x0B
	BRBS	1, Once
	CPI		R22, 0xC
	BRBS	1, Doce
	CPI		R22, 0x0D
	BRBS	1, Trece
	CPI		R22, 0x0E
	BRBS	1, Catorce
	CPI		R22, 0x0F
	BRBS	1, Quince
	RET
Cero:
	LDI		R26, 0b00111111
	OUT		PORTD, R26
	RET
Uno:
	LDI		R26, 0b00000110
	OUT		PORTD, R26
	RET
Dos:
	LDI		R26, 0b01011011
	OUT		PORTD, R26
	RET
Tres:
	LDI		R26, 0b01001111
	OUT		PORTD, R26
	RET
Cuatro:
	LDI		R26, 0b01100110
	OUT		PORTD, R26
	RET
Cinco:
	LDI		R26, 0b01101101
	OUT		PORTD, R26
	RET
Seis:
	LDI		R26, 0b01111101
	OUT		PORTD, R26
	RET
Siete:
	LDI		R26, 0b00000111
	OUT		PORTD, R26
	RET
Ocho:
	LDI		R26, 0b01111111
	OUT		PORTD, R26
	RET
Nueve:
	LDI		R26, 0b01101111
	OUT		PORTD, R26
	RET
Diez:
	LDI		R26, 0b01110111
	OUT		PORTD, R26
	RET
Once:
	LDI		R26, 0b01111100
	OUT		PORTD, R26
	RET
Doce:
	LDI		R26, 0b00111001
	OUT		PORTD, R26
	RET
Trece:
	LDI		R26,0b01011110
	OUT		PORTD, R26
	RET
Catorce:
	LDI		R26, 0b01111001
	OUT		PORTD, R26
	RET
Quince:
	LDI		R26, 0b01110001
	OUT		PORTD, R26
	RET

/****************************************/
// Interrupt routines

/****************************************/
//Interrupción puerto B
PCINT0_ISR:
	PUSH	R16	//Guardar constante para TNT0
	PUSH	R22 //Guardar contador display
	PUSH	R23 //Guardar Contador overflows
	IN		R16, SREG //Guardar sreg
	PUSH	R16

	INC		R18 //Sumar 1 al contador

	POP		R16 //Cargar todo lo guardado
	OUT		SREG, R16
	POP		R23
	POP		R22
	POP		R16
	RETI

//Interrupción en puerto C
PCINT1_ISR:
	PUSH	R16	//Guardar constante para TNT0
	PUSH	R22 //Guardar contador display
	PUSH	R23 //Guardar Contador overflows
	IN		R16, SREG //Guardar sreg
	PUSH	R16

	DEC		R18 //Restar 1 al contador

	POP		R16 //Cargar todo lo guardado
	OUT		SREG, R16
	POP		R23
	POP		R22
	POP		R16
	RETI