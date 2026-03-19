/*
* contador.asm
*
* Creado: 
* Autor : 
* Descripción: 
*/
/****************************************/
// Encabezado (Definición de Registros, Variables y Constantes)
.include "M328PDEF.inc"     // Include definitions specific to ATMega328P
.equ	T1VALUE = 0xE17B
.equ	MAX_MODES = 4
.def	MODE = R20 
.def	COUNTER = R21
.def	ACCION = R22
.dseg
.org    SRAM_START
//variable_name:     .byte   1   // Memory alocation for variable_name:     .byte   (byte size)

.cseg
.org 0x0000
	JMP SETUP
.org PCI2addr
	JMP PCINT2_ISR
.org OVF1addr
	JMP	TMR1_ISR

 /****************************************/
// Configuración de la pila
LDI     R16, LOW(RAMEND)
OUT     SPL, R16
LDI     R16, HIGH(RAMEND)
OUT     SPH, R16
/****************************************/
// Configuracion MCU
SETUP:
	CLI
	LDI R16, (1 << CLKPCE)
	STS CLKPR, R16
	LDI R16, (1 << CLKPS2)
	STS CLKPR, R16

	SBI DDRB, DDB0
	SBI DDRB, DDB1
	CBI PORTB, PORTB0
	CBI PORTB, PORTB1
	LDI R16, 0xFF
	OUT DDRC, R16
	LDI R16, 0x00
	OUT PORTC, R16
	CALL INIT_Timer1

	CBI DDRD, DDD2
	CBI DDRD, DDD3
	SBI PORTD, PORTD2
	SBI PORTD, PORTD3

	CLR MODE
	CLR ACCION
	CLR COUNTER

	LDI R16, (1 << TOIE1)
	STS TIMSK1, R16
	LDI R16, (1 << PCIE2)
	STS PCICR, R16
	LDI R16, (1 << PCINT19)|(1 << PCINT20)
	STS PCMSK2, R16

/****************************************/
// Loop Infinito LOGICA PULL-UP, INVERTIDA
MAIN_LOOP:
	OUT PORTC, COUNTER
	OUT PORTB, MODE
	CPI MODE, 0
	BREQ RUT0
	CPI MODE, 1
	BREQ RUT1
	CPI MODE, 2
	BREQ RUT2
	CPI MODE, 3
	BREQ RUT3
	RJMP MAIN_LOOP

RUT0:
	CPI ACCION, 0x01
	BRNE EXIT0
	INC COUNTER
	CLR ACCION
	RJMP MAIN_LOOP

RUT1:
	CPI ACCION, 0x01
	BRNE EXIT1
	DEC COUNTER
	CLR ACCION
	RJMP MAIN_LOOP

RUT2:
	CPI ACCION, 0x01
	BRNE EXIT0
	INC COUNTER
	CLR ACCION
	RJMP MAIN_LOOP

RUT3:
	CPI ACCION, 0x01
	BRNE EXIT1
	DEC COUNTER
	CLR ACCION
	RJMP MAIN_LOOP

EXIT0:
	RJMP MAIN_LOOP
EXIT1:
	RJMP MAIN_LOOP
EXIT2:
	RJMP MAIN_LOOP
EXIT3:
	RJMP MAIN_LOOP


/****************************************/
// NON-Interrupt subroutines
INIT_Timer1:
	LDI R16, 0x00
	STS TCCR1A, R16
	LDI	R16, (1 << CS11)|(1<<CS10)
	STS TCCR1B, R16
	LDI R16, HIGH(T1VALUE)
	STS TCNT1H, R16
	LDI R16, LOW(T1VALUE)
	STS TCNT1L, R16
	RET
/****************************************/
// Interrupt routines
PCINT2_ISR:
	SBIS PIND, PIND2
	INC MODE
	CPI MODE, MAX_MODES
	BRNE CONTINUE
	CLR MODE
CONTINUE:
	CPI MODE, 2
	BREQ EXIT_PCINT2
	CPI MODE, 3
	BREQ EXIT_PCINT2
	SBIS PIND, PIND3
	LDI ACCION, 0x01
EXIT_PCINT2:
	RETI
TMR1_ISR:
	LDI R16, HIGH(T1VALUE)
	STS TCNT1H, R16
	LDI R16, LOW(T1VALUE)
	STS TCNT1L, R16
	CPI MODE, 0
	BREQ EXIT_tm1
	CPI MODE, 1
	BREQ EXIT_tm1
	LDI ACCION, 0x01
EXIT_tm1:
	RETI
/****************************************/