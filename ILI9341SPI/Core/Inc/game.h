/*
 * game.h
 *
 * Base para pantalla Ili
 */
#ifndef INC_GAME_H_
#define INC_GAME_H_

#include <stdint.h>
#include "graphics.h"

//Pantalla (portrait)
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

//sprite de Mario, 16x32)
#define PLAYER_W       MARIO_W
#define PLAYER_H       MARIO_H

#define COLOR_BG       0x0000

//Física
#define MOVE_SPEED     2   // px por tick logico de 16 ms
/* Velocidad vertical y gravedad en punto fijo: 256 unidades = 1 pixel.
 * Altura libre conservada: 91 px*/
#define PHYSICS_SCALE  256
#define GRAVITY        48     // ajustada a tick de 16 ms; conserva aceleracion por segundo
#define JUMP_VELOCITY  (-1544) // mitad de velocidad/tick para 60 Hz logicos
#define MAX_FALL_SPEED 1024   // 4 px/tick = misma velocidad terminal por segundo

// Flags internos de entrada USART3
extern volatile uint8_t g_moveLeft;
extern volatile uint8_t g_moveRight;
extern volatile uint8_t g_jumpRequest;

// Inicializa el estado del jugador y pinta el fondo negro + jugador.
void Game_Init(void);

// Arma la recepcion de paquetes ASCII de dos bytes por USART3.
void Game_SerialInit(void);

// Actualiza entrada, IA y fisica. Llamar aproximadamente cada 16 ms.
// No realiza transferencias a la LCD
void Game_Update(void);

// Renderiza las regiones acumuladas. Llamar aproximadamente cada 32 ms.
void Game_Render(void);

#endif /* INC_GAME_H_ */
