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
	LDI R17, 0x00 //Variable de contador 1
	LDI R24, 0x00 //Vairable congtador 2
	LDI R22, 0x1F //Configugar port C 0-4 pins salida
	LDI R18, 0b11110000 //Pines a habilitar pullup de D
	LDI R23, 0x20 //pin habilitar pullup en c
	OUT DDRB, R16	//Configura pins 0-3 port b como salida
	OUT DDRC, R22 //Configura pins 0-4 port c como salida
	OUT DDRD, R16 //Configura pins port d como entrada y salida
	OUT PORTB, R17 //Apaga todos los pines de port b
	OUT PORTD, R18 //habilita pullup pins 4,5,6,7 port D
	OUT PORTC, R23 //habilitar pullups en C
	LDI R16, 0xF0 //constante para sumar a contador D
	LDI R26, 0x20 //constante para sumar a sumador C
/****************************************/
// Loop Infinito LOGICA PULL-UP, INVERTIDA
MAIN_LOOP:
	CALL	Contador_one
	CALL	Contador_two
	CALL	SUMADOR
	RJMP MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines

Contador_one:
	IN		R20, PIND //lee pines D
	ANDI	R20, 0x30 //Máscara botones contador 1 0b00110000
	CALL	ANTI_REBOTE //llama antirebote si se presiono algo
	CP		R20, R21 //compara ambos estados de pind, si son iguales continua, si no regresa al inicio.
	BRNE	Retorno
	CPI		R20, 0x00 // Si se presionan los botones a la vez, no hace nada
	BRBS	1, Retorno
	CPI		R20, 0x30 // Si no se presiona nada, no haga nada
	BRBS	1, Retorno
	CPI		R20, 0x10 //Verifica si se presiono el botón de suma, si sí, envía a suma
	BRBS	1, SUMA
	CPI		R20, 0x20 //Verifica si se presiona el botón de resta, si sí, evía a resta
	BRBS	1, RESTA
	RET

Contador_two:
	IN		R20, PIND //lee pines D
	ANDI	R20, 0xC0 //Máscara botonoes contador 2
	CALL	ANTI_REBOTE_two //llama antirebote si se presiono algo
	CP		R20, R21 //compara ambos estados de pind, si son iguales continua, si no regresa al inicio.
	BRNE	Retorno
	CPI		R20, 0x00 // Si se presionan los botones a la vez, no hace nada
	BRBS	1, Retorno
	CPI		R20, 0xC0 // Si se presionan los botones a la vez, no hace nada
	BRBS	1, Retorno
	CPI		R20, 0x80 //Verifica si se presiono el botón de suma, si sí, envía a suma
	BRBS	1, SUMA_two
	CPI		R20, 0x40 //Verifica si se presiona el botón de resta, si sí, evía a resta
	BRBS	1, RESTA_two
	RET

SUMADOR:
	IN		R20, PINC //lee pin C
	ANDI	R20, 0x20 //Máscara boton del sumador
	CALL	ANTI_REBOTE_three //llama antirebote si se presiono algo
	CP		R20, R21 //compara ambos estados de pinc, si son iguales continua, si no regresa al inicio.
	BRNE	Retorno
	CPI		R20, 0x00 // Si se presionó, avanza
	BRNE	Retorno
	MOV		R25, R24	//Copia el registro contador 2
	ANDI	R25, 0x0F	//Máscara de los 4 bits menos sigmificativos
	ADD		R25, R17	//Sumarle contador 1 al contador 2
	ADD		R25, R26	//Sumarle 0x20 para mantener pullup en pin 5 C
	OUT		PORTC, R25	//Actualizar port
	CALL	ANTI_FLANCO_three //antiflanco para no realizar más de una accion por presión del botón
	RET
	

Retorno:
	RET 
SUMA:
	INC	R17		//Suma 1 a r17
	ANDI R17, 0x0F //Máscara para solo primeros 4 bits
	OUT PORTB, R17//actualiza las salidas de portb
ANTI_FLANCO_SUMA: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	ANDI R20, 0x30
	CPI R20, 0x30
	BRNE ANTI_FLANCO_SUMA
	RET

SUMA_two:
	INC	R24		//Suma 1 a r24
	ANDI R24, 0x0F //Máscara para solo primeros 4 bits
	ADD R24, R16 //Asegura que los pines de entrada (4-7) tenga pullup.
	OUT PORTD, R24//actualiza las salidas de portb
ANTI_FLANCO_SUMA_two: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	ANDI R20, 0xC0
	CPI R20, 0xC0
	BRNE ANTI_FLANCO_SUMA_two
	RET

RESTA:
	DEC	R17 //Resta 1 a r17
	ANDI R17, 0x0F //Máscara para solo primeros 4 bits
	OUT PORTB, R17//Actualiza la salida
ANTI_FLANCO_RESTA: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	ANDI R20, 0x30
	CPI R20, 0x30
	BRNE ANTI_FLANCO_RESTA
	RET

RESTA_two:
	DEC	R24 //Resta 1 a r17
	ANDI R24, 0x0F //Máscara para solo primeros 4 bits
	ADD R24, R16 //Asegura que los pines de entrada D4-D7 tenga pullup.
	OUT PORTD, R24//Actualiza la salida
ANTI_FLANCO_RESTA_two: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PIND
	ANDI R20, 0xC0
	CPI R20, 0xC0
	BRNE ANTI_FLANCO_RESTA_two
	RET

ANTI_REBOTE:
	LDI R22, 255 //Seteo delay
	LDI R23, 255
LOOP_DELAY:
	DEC R22	//Va disminuyendo R22
	BRNE LOOP_DELAY
	DEC R23
	BRNE LOOP_DELAY
	IN R21, PIND //guarda el estado de oind en otro registro
	ANDI R21, 0x30
	RET

ANTI_REBOTE_two:
	LDI R22, 255 //Seteo delay
	LDI R23, 255
LOOP_DELAY_two:
	DEC R22	//Va disminuyendo R22
	BRNE LOOP_DELAY_two
	DEC R23
	BRNE LOOP_DELAY_two
	IN R21, PIND //guarda el estado de oind en otro registro
	ANDI R21, 0xC0
	RET

ANTI_REBOTE_three:
	LDI R22, 255 //Seteo delay
	LDI R23, 255
LOOP_DELAY_three:
	DEC R22	//Va disminuyendo R22
	BRNE LOOP_DELAY_three
	DEC R23
	BRNE LOOP_DELAY_three
	IN R21, PINC //guarda el estado de oind en otro registro
	ANDI R21, 0x20
	RET

ANTI_FLANCO_three: //detecta si se mantiene presionado el boton, si se matiene presionado queda em blucle hasta que se deje de presionar.
	IN R20, PINC
	ANDI R20, 0x20
	CPI R20, 0x20
	BRNE ANTI_FLANCO_three
	RET

/****************************************/
// Interrupt routines

/****************************************/