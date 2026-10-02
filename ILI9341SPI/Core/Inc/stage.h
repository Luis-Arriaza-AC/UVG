#ifndef INC_STAGE_H_
#define INC_STAGE_H_
#include <stdint.h>
#define STAGE_WIDTH 240
#define STAGE_HEIGHT 320
#define STAGE_FLOOR_Y 304
/* Devuelve el fondo RGB565 en coordenadas de pantalla. */
uint16_t Stage_Pixel(int x, int y);
#endif
