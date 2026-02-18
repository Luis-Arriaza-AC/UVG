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
	LDI		R16, (1<<CLKPS1)|(1<<CLKPS0) //Dividir entre 8
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
	LDI		R22,	0x00 //Registro contador display
	LDI		R23,	0x00 //registro contador overflows timer

	//Timer
	LDI		R16,	0x00 //Timer0 modo normal
	OUT		TCCR0A,	R16
	LDI		R16,	(1<<CS02)|(1<<CS00) //Prescaler timer 1024
	OUT		TCCR0B, R16
	LDI		R18,	0x00 //Registro de contador
	LDI		R16, 61 //Precargar 61 a TCNT0
	OUT		TCNT0, R16

/****************************************/
// Loop Infinito
MAIN_LOOP:
	CALL	Contador
	CALL	Boton
	CALL	DISPLAY
	CALL	ANTIFLANCO_check
	CALL	Igualdad
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines
Contador:
	SBIS	TIFR0, TOV0	//Verificar overflow
	RET
	SBI		TIFR0, TOV0 // Limpiar bandera de overflow
	OUT		TCNT0, R16//cargar 61 a tcnt0
	INC		R23	//Incrementa overflows
	CPI		R23, 10
	BRNE	Retorno
	LDI		R23,	0x00 //Resetea contador
	INC		R18 //Incrementar contador binario
	ANDI	R18, 0x0F //Máscara para solo los primero 4 bits
	OUT		PORTB, R18 //Mostrar en PORTB
	RET

Boton:
	CPI		R21,	0x00	//Leer registro antiflanco
	BRNE	Retorno		//Regresa si registro antiflanco activo
	IN		R20,	PINC	//Leer pin C
	ANDI	R20,	0x18	//Máscara
	CPI		R20,	0x18	//Si es ambos regresa
	BRBS	1,		Retorno
	CPI		R20,	0x08	//´Soló boton de suma
	BRBS	1,		Suma
	CPI		R20,	0x10	//Sólo botón de resta
	BRBS	1,		Resta
	RET

SUMA:		//Suma al contador del display y acitiva registro de antiflanco
	INC		R22
	ANDI	R22, 0x0F
	LDI		R21, 0x01
	RET

RESTA:		//Resta al contador del display y acitiva registro de antiflanco
	DEC		R22
	ANDI	R22, 0x0F
	LDI		R21, 0x01
	RET

Igualdad:
	CP		R22, R18	//Compara contadores, si son iguales avanza
	BRNE	Retorno
	SBI		PINC, 0 //toggle led alarma
	LDI		R18, 0x00 //reset contador binario
	OUT		PORTB, R18 
	RET

Retorno:
	RET

ANTIFLANCO_check:
	IN		R20, PINC
	ANDI	R20, 0x18
	CPI		R20, 0x00
	BRNE	Retorno
	LDI		R21, 0x00 //Borrar registro antiflanco si se soltó ambos botones
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
	LDI		R26, 0b00001111
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