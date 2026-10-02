/*
 * mario.h
 *
 * Sprite de Mario 16x32 (RGB565). Color transparente/fondo: 0x4208
 * Los datos reales están en mario.c (aquí solo se declara como extern
 * para evitar 'multiple definition' al incluir este header en varios .c)
 */
#ifndef INC_MARIO_H_
#define INC_MARIO_H_

#include <stdint.h>

#define MARIO_W 16
#define MARIO_H 32
#define MARIO_TRANSPARENT_COLOR 0x4208

extern const uint16_t marioSprite[MARIO_W * MARIO_H];

#endif /* INC_MARIO_H_ */
