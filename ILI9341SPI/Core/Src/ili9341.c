/*
 * ili9341.c
 *
 *  Created on: Aug 20, 2024
 *      Author: Pablo Mazariegos
 *
 *  MODIFICADO: orientación PORTRAIT (240x320) para base de videojuego.
 */
#include <stdlib.h> // malloc()
#include <string.h> // memset()
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "pgmspace.h"
#include "ili9341.h"
#include "main.h"

/*  RST 	- PC1
 *  RS/DC 	- PA4
 *  CS  	- PB0
 *  MOSI 	- PA7
 *  MISO 	- PA6
 *  SCK 	- PA5
 * */

#define LCD_DC_PORT    GPIOA
#define LCD_DC_PIN     GPIO_PIN_4
#define LCD_DC_L()     (LCD_DC_PORT->BSRR = (LCD_DC_PIN<<16))	//
#define LCD_DC_H()     (LCD_DC_PORT->BSRR =  LCD_DC_PIN)		//Poner el modo RS en high es para mandar dato

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_0
#define LCD_CS_L()     (LCD_CS_PORT->BSRR = (LCD_CS_PIN<<16)) 	//Pone el chip select en activo para empezar la comunicación con la pantalla
#define LCD_CS_H()     (LCD_CS_PORT->BSRR =  LCD_CS_PIN)		//

// ==================== Resolución de pantalla (PORTRAIT) ====================
// Antes el driver estaba inicializado en landscape (320x240). Para el juego
// se necesita portrait: 240 de ancho x 320 de alto.
#define LCD_WIDTH   240
#define LCD_HEIGHT  320

extern const uint8_t smallFont[1140];
extern const uint16_t bigFont[1520];
extern SPI_HandleTypeDef hspi1;
//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {

	//****************************************
	// Secuencia de Inicialización
	//****************************************
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	/*Configure GPIO pin Output Level */
	//HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin | LCD_WR_Pin | LCD_RS_Pin,GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(150);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	//****************************************
	LCD_CMD(0xE9);  // SETPANELRELATED
	LCD_DATA(0x20);
	//****************************************
	LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
	HAL_Delay(100);
	//****************************************
	LCD_CMD(0xD1);    // (SETVCOM)
	LCD_DATA(0x00);
	LCD_DATA(0x71);
	LCD_DATA(0x19);
	//****************************************
	LCD_CMD(0xD0);   // (SETPOWER)
	LCD_DATA(0x07);
	LCD_DATA(0x01);
	LCD_DATA(0x08);
	//****************************************
	// MEMORYACCESS (MADCTL) - orientación de pantalla
	// bit7 MY | bit6 MX | bit5 MV | bit4 ML | bit3 BGR | bit2 MH
	// Antes: 0x40|0x80|0x20|0x08 = MY+MX+MV+BGR -> LANDSCAPE (320x240)
	// Ahora: se quita MV (bit5) para dejarlo en PORTRAIT (240x320).
	// Si la imagen sale espejeada o al revés en tu montaje físico,
	// prueba estos otros valores: 0x08, 0x48, 0x88, 0xC8
	LCD_CMD(0x36);  // (MEMORYACCESS)
	LCD_DATA(0x40 | 0x80 | 0x08); // MY + MX + BGR  -> PORTRAIT
	//****************************************
	LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
	LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
	//****************************************
	LCD_CMD(0xC1);    // (POWERCONTROL2)
	LCD_DATA(0x10);
	LCD_DATA(0x10);
	LCD_DATA(0x02);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
	LCD_DATA(0x00);
	LCD_DATA(0x35);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x01);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
	LCD_DATA(0x04); // 72Hz
	//****************************************
	LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
	LCD_DATA(0x01);
	LCD_DATA(0x44);
	//****************************************
	LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
	LCD_DATA(0x04);
	LCD_DATA(0x67);
	LCD_DATA(0x35);
	LCD_DATA(0x04);
	LCD_DATA(0x08);
	LCD_DATA(0x06);
	LCD_DATA(0x24);
	LCD_DATA(0x01);
	LCD_DATA(0x37);
	LCD_DATA(0x40);
	LCD_DATA(0x03);
	LCD_DATA(0x10);
	LCD_DATA(0x08);
	LCD_DATA(0x80);
	LCD_DATA(0x00);
	//****************************************
	// CASET / PASET ajustados a PORTRAIT (240 ancho x 320 alto).
	// (En el original estaban invertidos y PASET tenía un valor
	// fuera de rango: 0x01E0 = 480, que no corresponde a 240 ni 320).
	LCD_CMD(0x2A); // Set_column_address 240px (CASET)
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA((LCD_WIDTH - 1) >> 8);
	LCD_DATA((LCD_WIDTH - 1) & 0xFF);
	//****************************************
	LCD_CMD(0x2B); // Set_page_address 320px (PASET)
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA((LCD_HEIGHT - 1) >> 8);
	LCD_DATA((LCD_HEIGHT - 1) & 0xFF);
	LCD_CMD(0x29); //display on
	LCD_CMD(0x2C); //display on

	LCD_CMD(ILI9341_INVOFF); //Invert Off
	HAL_Delay(120);
	LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
	HAL_Delay(120);
	LCD_CMD(ILI9341_DISPON);    //Display on
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
	LCD_CS_L();
	LCD_DC_L();
	// ESPERAR que SPI no esté ocupado PRIMERO
	while (SPI1->SR & SPI_SR_BSY)
		;
	// Versión simple LL
	while (!(SPI1->SR & SPI_SR_TXE))
		;  // Esperar TX empty
	SPI1->DR = cmd;                    // Escribir dato
	while (SPI1->SR & SPI_SR_BSY)
		;     // Esperar fin

	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
	LCD_CS_L();
	LCD_DC_H();
	HAL_SPI_Transmit(&hspi1, &data, 1, 1);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2,
		unsigned int y2) {
	LCD_CMD(0x2a); // Set_column_address 4 parameters
	LCD_DATA(x1 >> 8);
	LCD_DATA(x1);
	LCD_DATA(x2 >> 8);
	LCD_DATA(x2);
	LCD_CMD(0x2b); // Set_page_address 4 parameters
	LCD_DATA(y1 >> 8);
	LCD_DATA(y1);
	LCD_DATA(y2 >> 8);
	LCD_DATA(y2);
	LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
	unsigned int x, y;
	LCD_DC_H();
	LCD_CS_L();
	SetWindows(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

	for (x = 0; x < LCD_WIDTH; x++)
		for (y = 0; y < LCD_HEIGHT; y++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);
		}
	LCD_CS_H();

}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	LCD_DC_H();
	LCD_CS_L();
	SetWindows(x, y, l + x, y);
	for (i = 0; i < l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	LCD_DC_H();
	LCD_CS_L();
	SetWindows(x, y, x, y + l);
	for (i = 1; i <= l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	H_line(x, y, w, c);
	H_line(x, y + h, w, c);
	V_line(x, y, h, c);
	V_line(x + w, y, h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	LCD_CMD(0x02c); // write_memory_start
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + w - 1, y + h - 1);
	unsigned int k = w * h * 2 - 1;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);
			k = k - 2;
		}
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//***************************************************************************************************************************************
void LCD_Print(char *text, int x, int y, int fontSize, int color,
		int background) {

	int fontXSize;
	int fontYSize;

	if (fontSize == 1) {
		fontXSize = fontXSizeSmal;
		fontYSize = fontYSizeSmal;
	}
	if (fontSize == 2) {
		fontXSize = fontXSizeBig;
		fontYSize = fontYSizeBig;
	}
	if (fontSize == 3) {
		fontXSize = fontXSizeNum;
		fontYSize = fontYSizeNum;
	}

	char charInput;
	int cLength = strlen(text);
	int charDec;
	int c;
	char char_array[cLength + 1];
	for (int i = 0; text[i] != '\0'; i++) {
		char_array[i] = text[i];
	}

	for (int i = 0; i < cLength; i++) {
		charInput = char_array[i];
		charDec = (int) charInput;
		LCD_CS_L();
		SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1,
				y + fontYSize);
		long charHex1;
		for (int n = 0; n < fontYSize; n++) {
			if (fontSize == 1) {
				charHex1 = pgm_read_word_near(
						smallFont + ((charDec - 32) * fontYSize) + n);
			}
			if (fontSize == 2) {
				charHex1 = pgm_read_word_near(
						bigFont + ((charDec - 32) * fontYSize) + n);
			}
			for (int t = 1; t < fontXSize + 1; t++) {
				if ((charHex1 & (1 << (fontXSize - t))) > 0) {
					c = color;
				} else {
					c = background;
				}
				LCD_DATA(c >> 8);
				LCD_DATA(c);
			}
		}
		LCD_CS_H();
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height, const uint16_t *bitmap) {
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + width - 1, y + height - 1);
	unsigned int total_pixels = width * height;

	for (unsigned int i = 0; i < total_pixels; i++) {
		uint16_t pixel = bitmap[i];
		LCD_DATA(pixel >> 8);
		LCD_DATA(pixel & 0xFF);
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits) con color transparente
//***************************************************************************************************************************************
void LCD_BitmapTransparent(uint16_t x, uint16_t y, uint16_t width,
		uint16_t height, const uint16_t *bitmap, uint16_t transparentColor) {
	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			unsigned int index = j * width + i;
			uint16_t pixel = bitmap[index];

			if (pixel != transparentColor) {
				LCD_CS_L();
				SetWindows(x + i, y + j, x + i, y + j);
				LCD_CMD(0x02c);
				LCD_DC_H();
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				LCD_CS_H();
			}
		}
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, const uint16_t *bitmap,
		int columns, int index, char flip, char offset) {
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + width - 1, y + height - 1);

	int ancho = width * columns;
	int k = 0;

	if (flip) {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width - 1 - offset;
			k = k + width;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				k--;
			}
		}
	} else {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width + 1 + offset;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				k++;
			}
		}
	}

	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar un sprite con "transparencia" de forma RÁPIDA y sin flicker.
// A diferencia de LCD_BitmapTransparent (que abre una ventana SPI por cada píxel,
// muy lento), esta función abre la ventana UNA sola vez para todo el sprite y
// sustituye los píxeles del color "transparentColor" por bgColor al vuelo.
// flipH = 1 dibuja el sprite espejado horizontalmente (para mirar a la izquierda).
//***************************************************************************************************************************************
void LCD_BitmapMasked(int x, int y, int w, int h, const uint16_t *bitmap,
		uint16_t transparentColor, uint16_t bgColor, uint8_t flipH) {
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + w - 1, y + h - 1);

	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			int srcCol = flipH ? (w - 1 - col) : col;
			uint16_t pixel = bitmap[row * w + srcCol];
			if (pixel == transparentColor) {
				pixel = bgColor;
			}
			LCD_DATA(pixel >> 8);
			LCD_DATA(pixel & 0xFF);
		}
	}

	LCD_CS_H();
}
