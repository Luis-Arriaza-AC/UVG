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
	LDI R16, 0x0F //Variables de setup pines
	LDI R17, 0x00 //Variable de contador
	LDI R18, 0b00110000 //Pines a habilitar pullup de D
	OUT DDRB, R16	//Configura pins 0-3 port b como salida
	OUT DDRD, R17 //Configura pins port d como entrada
	OUT PORTB, R17 //Apaga todos los pines de port b
	OUT PORTD, R18 //habilita pullup pins 4 y 5 port D
/****************************************/
// Loop Infinito LOGICA PULL-UP, INVERTIDA
MAIN_LOOP:
	IN		R20, PIND //lee pines D
	CPI		R20, 0x30 //Verfifica si no es presiona nada
	BRBS	1, MAIN_LOOP //Regresa sino se se presionaron
	CALL	ANTI_REBOTE //llama antirebote si se presiono algo
	CP		R20, R21 //compara ambos estados de pind, si son iguales continua, si no regresa al inicio.
	BRNE	MAIN_LOOP
	CPI		R20, 0x00 // Si se presionan los botones a la vez, no hace nada
	BRBS	1, MAIN_LOOP
	CPI		R20, 0x20 //Verifica si se presiono el botón de suma, si sí, envía a suma
	BRBS	1, SUMA
	CPI		R20, 0x10 //Verifica si se presiona el botón de resta, si sí, evía a resta
	BRBS	1, RESTA
	RJMP	MAIN_LOOP
/****************************************/
// NON-Interrupt subroutines

ANTI_REBOTE:
	LDI R22, 255 //Seteo delay
	LDI R23, 100
LOOP_DELAY:
	DEC R22	//Va disminuyendo R22
	BRNE LOOP_DELAY
	DEC R23
	BRNE LOOP_DELAY
	IN R21, PIND //guarda el estado de oind en otro registro
	RET

SUMA:
	INC	R17		//Suma 1 a r17
	ANDI R17, 0x0F //Máscara para solo primeros 4 bits
	OUT PORTB, R17//actualiza las salidas de portb
ANTI_FLANCO_SUMA: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	CPI R20, 0x30
	BRNE ANTI_FLANCO_SUMA
	JMP MAIN_LOOP

RESTA:
	DEC	R17 //Resta 1 a r17
	ANDI R17, 0x0F //Máscara para solo primeros 4 bits
	OUT PORTB, R17//Actualiza la salida
ANTI_FLANCO_RESTA: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	CPI R20, 0x30
	BRNE ANTI_FLANCO_RESTA
	JMP MAIN_LOOP


/****************************************/
// Interrupt routines

/****************************************/