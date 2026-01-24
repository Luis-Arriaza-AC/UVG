/*
* NombreProgra.asm
*
* Creado: 
* Autor : 
* Descripción: 
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
	LDI R16, 0x10 //Valor para D4
	OUT DDRD, R16 //Configurar D4 para salida
	LDI R17, 0x00 //Valor para reset
	LDI R18, 0x00
	LDI R19, 0x00
/****************************************/
// Loop Infinito
MAIN_LOOP:
	OUT PORTD, R16 //Encender para D4
	CALL Delay //Llamar delay
	OUT PORTD, R17 //Apagar D4
	CALL Delay //Llamar delay
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines
Delay:
	INC R18	//Va incrementando R18
	BRNE Delay
	INC R19 //Cada vez que hay overflow en R18 incrementa en 1 R19
	BRNE Delay
	INC R20 //Cada vez que hay overflow en R19 incrementa en 1 R20
	BRNE Delay
	RET //Regresa hasta que se llene R20
/****************************************/
// Interrupt routines

/****************************************/