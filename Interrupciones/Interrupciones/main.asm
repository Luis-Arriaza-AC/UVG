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
JMP SETUP

.org 0x0006 // Vector PCINT0 Puerto B
JMP PCINT0_ISR

.org 0x0008 // Vector PCINT1 Puerto C
JMP PCINT1_ISR

.org 0x0012   // Vector TIMER2_OVF
JMP  TIM2_OVF_ISR

.org 0x0020 //Vector interrupcion timer0 overflow
JMP TIM0_OVF_ISR



 /****************************************/
// Configuración de la pila
LDI     R16, LOW(RAMEND)
OUT     SPL, R16
LDI     R16, HIGH(RAMEND)
OUT     SPH, R16
/****************************************/
// Configuracion MCU
SETUP:
	//Puertos
	LDI		R16,	0xFF //Todo puerto D salida para display
	OUT		DDRD,	R16
	LDI		R16,	0x03 //Pin 0-1 puerto C como salida transistores
	OUT		DDRC,	R16
	LDI		R16,	0x0F //Pin 0-3 puerto B salida leds contador botones
	OUT		DDRB,	R16
	LDI		R16,	0x00 //Apagar display
	OUT		PORTD,	R16
	LDI		R16,	0x10 //Encender pullup portB pin 4
	OUT		PORTB,	R16
	LDI		R16,	0x05 //Encender pullup portC pin 2 y dejar encendido display 2
	OUT		PORTC,	R16

	//Interrupciones
	LDI		R16, (1<<PCIE0)|(1<<PCIE1) // Habilitar PCINT en Puerto B y Puerto C
	STS		PCICR, R16
	LDI		R16, (1<<PCINT4) //Pin 4 de puerto B
	STS		PCMSK0, R16
	LDI		R16, (1<<PCINT10) //Pin 3 de puerto C
	STS		PCMSK1, R16
	LDI		R16, (1<<TOIE0) //Habilita interrupt de timer 0
	STS		TIMSK0, R16
	LDI		R16, (1<<TOIE2)
	STS		TIMSK2, R16 //Habilita interrupt timer 2
	SEI

	//Registros de contadores
	LDI		R22,	0x00 //Registro contador display 1
	LDI		R29,	0x00 //Registro contador display 2
	LDI		R23,	0x00 //registro contador overflows timer
	LDI		R18,	0x00 //Registro de contador leds


	//Timers
	LDI		R16, 0x00 //timer 2 modo normal
	STS		TCCR2A, R16         
	LDI		R16, (1<<CS22)|(1<<CS21)|(1<<CS20) //Prescaler timer 1024
	STS		TCCR2B, R16         
	LDI		R16,	0x00 //Timer0 modo normal
	OUT		TCCR0A,	R16
	LDI		R16,	(1<<CS02)|(1<<CS00) //Prescaler timer 1024
	OUT		TCCR0B, R16
	LDI		R16, 100 //Precargar 100 a TCNT0
	OUT		TCNT0, R16



/****************************************/
// Loop Infinito
MAIN_LOOP:
	CALL	Contador_leds
	CALL	Contador_disp
	//Display 1
	CBI		PORTC, 0
	CBI		PORTC, 1
	MOV		R20, R22
	CALL	DISPLAY
	SBI		PORTC, 1
	//Display 2
	CBI		PORTC, 1
	CBI		PORTC, 0
	MOV		R20, R29
	CALL	DISPLAY
	SBI		PORTC, 0
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines

Contador_leds:
	ANDI	R18, 0x0F //Máscara para solo primeros 4 bits
	ORI		R18, 0x10 //Manterner el pullup de el botón activado
	OUT		PORTB, R18
	RET

Retorno:
	RET

Contador_disp:
	CPI		R23, 100  //Comparar overflows del timer
	BRNE	Retorno
	LDI		R23, 0x00
	CPI		R22, 9
	BREQ	overflow1
	INC		R22	//Incrementa contador display
	ANDI	R22, 0x0F //Máscara solo primeros 4 bits
	RET

overflow1:
	LDI R22, 0x00
	CPI	R29, 5
	BREQ overflow2
	INC R29
	ANDI R29, 0x0F
	RET

overflow2:
	LDI R29, 0x00
	RET

DISPLAY:
	CPI		R20, 0x00
	BRBS	1, Cero
	CPI		R20, 0x01
	BRBS	1, Uno
	CPI		R20, 0x02
	BRBS	1, Dos
	CPI		R20, 0x03
	BRBS	1, Tres
	CPI		R20, 0x04
	BRBS	1, Cuatro
	CPI		R20, 0x05
	BRBS	1, Cinco
	CPI		R20, 0x06
	BRBS	1, Seis
	CPI		R20, 0x07
	BRBS	1, Siete
	CPI		R20, 0x08
	BRBS	1, Ocho
	CPI		R20, 0x09
	BRBS	1, Nueve
	CPI		R20, 0x0A
	BRBS	1, Diez
	CPI		R20, 0x0B
	BRBS	1, Once
	CPI		R20, 0xC
	BRBS	1, Doce
	CPI		R20, 0x0D
	BRBS	1, Trece
	CPI		R20, 0x0E
	BRBS	1, Catorce
	CPI		R20, 0x0F
	BRBS	1, Quince
	RET
Cero:
	LDI		R26, 0b00111111
	RJMP	Mostrar_disp
Uno:
	LDI		R26, 0b00000110
	RJMP	Mostrar_disp
Dos:
	LDI		R26, 0b01011011
	RJMP	Mostrar_disp
Tres:
	LDI		R26, 0b01001111
	RJMP	Mostrar_disp
Cuatro:
	LDI		R26, 0b01100110
	RJMP	Mostrar_disp
Cinco:
	LDI		R26, 0b01101101
	RJMP	Mostrar_disp
Seis:
	LDI		R26, 0b01111101
	RJMP	Mostrar_disp
Siete:
	LDI		R26, 0b00000111
	RJMP	Mostrar_disp
Ocho:
	LDI		R26, 0b01111111
	RJMP	Mostrar_disp
Nueve:
	LDI		R26, 0b01101111
	RJMP	Mostrar_disp
Diez:
	LDI		R26, 0b01110111
	RJMP	Mostrar_disp
Once:
	LDI		R26, 0b01111100
	RJMP	Mostrar_disp
Doce:
	LDI		R26, 0b00111001
	RJMP	Mostrar_disp
Trece:
	LDI		R26,0b01011110
	RJMP	Mostrar_disp
Catorce:
	LDI		R26, 0b01111001
	RJMP	Mostrar_disp
Quince:
	LDI		R26, 0b01110001
	RJMP	Mostrar_disp
Mostrar_disp:
	OUT		PORTD, R26
	RET

/****************************************/
// Interrupt routines

/****************************************/
//Interrupción puerto B
PCINT0_ISR:
    PUSH R16
    PUSH R17
    IN   R16, SREG
    PUSH R16
    IN   R17, PINB
    ANDI R17, 0x10      //Máscara para pin 4
    CPI  R24, 0x10      // verficar si antes estaba en 1
    BRNE fin_B          // Si no, regresa
    CPI  R17, 0x00      // Verficar si ahora esta en 0
    BRNE fin_B          // Si no, regresa
    INC  R18            
fin_B:
    MOV  R24, R17       // Guardar estado actual
	LDS  R16, PCMSK0
    ANDI R16, ~(1<<PCINT4)
    STS  PCMSK0, R16
    POP  R16
    OUT  SREG, R16
    POP  R17
    POP  R16
    RETI

//Interrupción en puerto C
PCINT1_ISR:
    PUSH R16
    PUSH R17
    IN   R16, SREG
    PUSH R16
    IN   R17, PINC
    ANDI R17, 0x04      //Máscara para pin 2
    CPI  R25, 0x04      // verficar si antes estaba en 1
    BRNE fin_C			// Si no, regresa
    CPI  R17, 0x00      // Verficar si ahora esta en 0
    BRNE fin_C			// Si no, regresa
    DEC  R18          
fin_C:
    MOV  R25, R17       // Guardar estado actual
	LDS  R16, PCMSK1
    ANDI R16, ~(1<<PCINT10)
    STS  PCMSK1, R16
    POP  R16
    OUT  SREG, R16
    POP  R17
    POP  R16
    RETI

//Interrupción timer 2
TIM2_OVF_ISR:
    PUSH R16
    IN   R16, SREG
    PUSH R16
	INC	 R30
	CPI	 R30, 6
	BRNE Reti_timer2
	LDI	 R30, 0x00
    LDS  R16, PCMSK0
    ORI  R16, (1<<PCINT4)
    STS  PCMSK0, R16
    LDS  R16, PCMSK1
    ORI  R16, (1<<PCINT10)
    STS  PCMSK1, R16
Reti_timer2:
	POP  R16
    OUT  SREG, R16
    POP  R16
    RETI

//Interrupción timer 0
TIM0_OVF_ISR:
	IN		R17, SREG //Guardar sreg
	PUSH	R17
	OUT		TCNT0, R16 //Seteo de TCNT0
	INC		R23 //Contador overflows
	POP		R17 //Cargar lo guardado
	OUT		SREG, R17
	RETI