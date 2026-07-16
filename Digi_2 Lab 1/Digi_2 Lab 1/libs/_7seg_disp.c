/*
 * _7seg_disp.c
 *
 * Created: 09/07/2026 18:02:47
 *  Author: luisz
 */ 
#include "_7seg_disp.h"

// Tabla para display de 7 segmentos catodo comun
static const uint8_t digitos[] = {
	0x3F, // 0 -> 0111111
	0x06, // 1 -> 0000110
	0x5B, // 2 -> 1011011
	0x4F, // 3 -> 1001111
	0x66, // 4 -> 1100110
	0x6D, // 5 -> 1101101
	0x7D, // 6 -> 1111101
	0x07, // 7 -> 0000111
	0x7F, // 8 -> 1111111
	0x6F, // 9 -> 1101111
	0x77, // A -> 1110111
	0x7C, // b -> 1111100
	0x39, // C -> 0111001
	0x5E, // d -> 1011110
	0x79, // E -> 1111001
	0x71  // F -> 1110001
};

void disp_apagar(void)
{
	PORTD = 0x00;
}

void disp_mostrar(uint8_t numero)
{
	uint8_t mapa;

	if (numero > 0x0F) {
		return;
	}
	else {
		mapa = digitos[numero];
		PORTD = mapa;
	}
}