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
    LDI R16, 0x00 //LIMPIAR R16
/****************************************/
// Loop Infinito
MAIN_LOOP:
	INC		R16
	CALL	delay
    RJMP    MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines
delay:
	LDI R17, 255
delay1:
	DEC R17
	BRNE delay1
	RET
/****************************************/
// Interrupt routines

/****************************************/