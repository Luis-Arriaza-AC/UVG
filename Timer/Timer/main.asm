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
	LDI		R16, (1<<CLKPS1)|(1<<CLKPS0) //Divide entre 8
	STS		CLKPR, R16

	//Puertos
	LDI		R16,	0xFF //Todo puerto D salida
	OUT		DDRD,	R16
	LDI		R16,	0x01 //Pin 0 puerto C salida
	OUT		DDRC,	R16
	LDI		R16,	0x0F //Pin 0-3 puerto B salida
	OUT		DDRB,	R16
	LDI		R16,	0x00 //Apagar todos los puertos
	OUT		PORTD,	R16
	OUT		PORTC,	R16
	OUT		PORTB,	R16

	//Timer
	LDI		R16,	0x00 //Timer modo normal
	OUT		TCCR0A,	R16
	LDI		R16,	(1<<CS02)|(1<<CS00) //Prescaler 1024
	OUT		TCCR0B, R16
	LDI		R18,	0x00 //Registro de contador
	LDI		R16, 61 //Precargar 61 TCNT0
	OUT		TCNT0, R16

/****************************************/
// Loop Infinito
MAIN_LOOP:
	SBIS	TIFR0, TOV0	//Verificar overflow
	RJMP	MAIN_LOOP
	SBI		TIFR0, TOV0 // Limpiar bandera de overflow
	OUT		TCNT0, R16
	INC		R18 //Incrementar contador binario
	ANDI	R18, 0x0F //Máscara para solo los primero 4 bits
	OUT		PORTB, R18 //Mostrar en PORTB
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines

/****************************************/
// Interrupt routines

/****************************************/