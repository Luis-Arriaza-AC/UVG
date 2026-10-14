#include "sonidos.h"
#include "main.h"
#include <stddef.h>

//TIM2 CH1 se configura en CubeMX con tick de 1 MHz
extern TIM_HandleTypeDef htim2;

typedef struct {
    uint16_t frecuencia;
    uint8_t duracion;
    uint8_t punto;
} Nota;

/* RTTTL decodificado: SuperMar:d=35,o=5,b=56 */
static const Nota musicaTitle[] = {
{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,1},{622,35,1},{659,35,1},{784,16,1},{659,35,1},{784,35,1},{880,8,0},{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,0},{1568,16,1},{1568,35,1},{1568,4,0},{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,1},{622,35,1},{659,35,1},{784,16,1},{659,35,1},{784,35,1},{880,8,0},{932,8,0},{880,8,0},{784,4,1},{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,1},{622,35,0},{659,35,1},{784,16,1},{659,35,1},{784,35,0},{880,8,0},{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,1},{1568,16,1},{1568,35,1},{1568,4,0},{1175,35,1},{1109,35,1},{1047,35,1},{932,16,1},{988,16,1},{622,35,1},{659,35,0},{784,16,1},{659,35,1},{784,35,1},{880,8,0},{932,8,0},{880,8,0}
};
/* RTTTL decodificado: Theme:d=4,o=6,b=100 */
static const Nota musicaInicio[] = {
{1319,16,0},{1319,16,0},{0,32,0},{1319,8,0},{1047,16,0},{1319,8,0},{1568,8,0},{0,8,0},{784,8,0},{0,8,0}
};
/* RTTTL decodificado: DeathMus:d=16,o=5,b=70 */
static const Nota musicaMuerte[] = {
{988,16,0},{1397,16,0},{0,16,0},{1397,16,0},{1397,16,0},{1397,16,1},{1319,16,1},{1175,16,0},{1047,16,0},{784,16,0},{0,16,0},{659,16,0},{523,16,0}
};

//Estructura para música
typedef struct {
    const Nota *notas;
    uint16_t cantidad;
    uint16_t indice;
    uint16_t bpm;
    uint8_t loop;
    uint8_t activa;
    uint8_t enPausa;
    uint32_t faseTick;
    uint32_t duracionNotaMs;
    SonidoId id;
} Reproductor;

static Reproductor rep;


void Sonidos_Init(void) {
    rep.activa = 0;
    rep.id = SONIDO_NINGUNO;
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}

static void PWM_Frecuencia(uint16_t freq) {
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    if (freq != 0U) {
        uint32_t periodo = 1000000UL / (uint32_t)freq;
        if (periodo < 2U) periodo = 2U;
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        __HAL_TIM_SET_AUTORELOAD(&htim2, periodo - 1U);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, periodo / 2U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    }
}

static uint32_t DuracionMs(const Nota *n, uint16_t bpm) {
    uint32_t redonda = (60000UL * 4UL) / bpm;
    uint32_t ms = redonda / n->duracion;
    if (n->punto) ms += ms / 2U;
    return ms ? ms : 1U;
}

static void IniciarNota(uint32_t now) {
    const Nota *n = &rep.notas[rep.indice];
    rep.duracionNotaMs = DuracionMs(n, rep.bpm);
    rep.faseTick = now;
    rep.enPausa = 0;
    PWM_Frecuencia(n->frecuencia);
}

//Reproducción de sonido pwm
static void Play(const Nota *notas, uint16_t cantidad, uint16_t bpm, uint8_t loop, SonidoId id) {
    Sonidos_Stop();
    rep.notas = notas;
    rep.cantidad = cantidad;
    rep.indice = 0;
    rep.bpm = bpm;
    rep.loop = loop;
    rep.activa = 1;
    rep.id = id;
    IniciarNota(HAL_GetTick());
}

//Reproducir las 3 diferentes canciones
void Sonidos_PlayTitle(void) {
    Play(musicaTitle, (uint16_t)(sizeof(musicaTitle)/sizeof(musicaTitle[0])), 56, 1, SONIDO_TITLE);
}
void Sonidos_PlayLevelStart(void) {
    Play(musicaInicio, (uint16_t)(sizeof(musicaInicio)/sizeof(musicaInicio[0])), 100, 0, SONIDO_INICIO_NIVEL);
}
void Sonidos_PlayDeath(void) {
    Play(musicaMuerte, (uint16_t)(sizeof(musicaMuerte)/sizeof(musicaMuerte[0])), 70, 0, SONIDO_MUERTE);
}
//Mutear pwm
void Sonidos_Stop(void) {
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    rep.activa = 0;
    rep.id = SONIDO_NINGUNO;
}
uint8_t Sonidos_IsPlaying(void) { return rep.activa; }
SonidoId Sonidos_Current(void) { return rep.id; }

void Sonidos_Update(void) {
    if (!rep.activa) return;
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = (uint32_t)(now - rep.faseTick);
    uint32_t onMs = (rep.duracionNotaMs * 9U) / 10U;
    if (!rep.enPausa && elapsed >= onMs) {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        rep.enPausa = 1;
    }
    if (elapsed < rep.duracionNotaMs) return;
    rep.indice++;
    if (rep.indice >= rep.cantidad) {
        if (rep.loop) rep.indice = 0;
        else { Sonidos_Stop(); return; }
    }
    IniciarNota(now);
}
