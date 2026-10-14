#ifndef INC_SONIDOS_H_
#define INC_SONIDOS_H_

#include <stdint.h>

typedef enum {
    SONIDO_NINGUNO = 0,
    SONIDO_TITLE,
    SONIDO_INICIO_NIVEL,
    SONIDO_MUERTE
} SonidoId;

//sonidos por pwm
void Sonidos_Init(void);
void Sonidos_Update(void);
void Sonidos_PlayTitle(void);
void Sonidos_PlayLevelStart(void);
void Sonidos_PlayDeath(void);
void Sonidos_Stop(void);
uint8_t Sonidos_IsPlaying(void);
SonidoId Sonidos_Current(void);

#endif
