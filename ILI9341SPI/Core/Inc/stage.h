#ifndef INC_STAGE_H_
#define INC_STAGE_H_
#include <stdint.h>
#define STAGE_WIDTH 240
#define STAGE_HEIGHT 320
#define STAGE_FLOOR_Y 304
/* Devuelve el fondo RGB565 en coordenadas de pantalla. */
uint16_t Stage_Pixel(int x, int y);
uint8_t Stage_IsSolid(int x, int y);
uint8_t Stage_IsPipe(int x, int y);
uint8_t Stage_GetPlatformSpan(int x, int y, int *left, int *right, int *top);
void Stage_SetLevel(uint16_t level);
#endif
