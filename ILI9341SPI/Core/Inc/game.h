/*
 * game.h
 *
 * Base de videojuego para ILI9341 (STM32F446, modo portrait 240x320).
 */
#ifndef INC_GAME_H_
#define INC_GAME_H_

#include <stdint.h>
#include "mario.h"

// ==================== Pantalla (portrait) ====================
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

// ==================== Jugador (sprite de Mario, 16x32) ====================
#define PLAYER_W       MARIO_W
#define PLAYER_H       MARIO_H

#define COLOR_BG       0x0000  // Negro

// ==================== Física ====================
#define MOVE_SPEED     4   // px por frame al mover izq/der
#define GRAVITY        1   // aceleración vertical por frame
#define JUMP_VELOCITY  (-12) // impulso inicial del salto (negativo = hacia arriba)
#define MAX_FALL_SPEED 10   // velocidad terminal de caída

// Flags de entrada. Se actualizan en HAL_GPIO_EXTI_Callback() (main.c)
// y se consumen en Game_Update().
extern volatile uint8_t g_moveLeft;
extern volatile uint8_t g_moveRight;
extern volatile uint8_t g_jumpRequest;

// Inicializa el estado del jugador y pinta el fondo negro + jugador.
void Game_Init(void);

// Actualiza física + entrada y redibuja solo lo que cambió (borra la
// posición anterior del jugador y dibuja la nueva). Llamar 1 vez por frame.
void Game_Update(void);

#endif /* INC_GAME_H_ */
