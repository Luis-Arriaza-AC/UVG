/*
 * servo.h  –  corregido
 *
 * Created: 15/05/2026 15:32:28
 *  Author: luisz
 */
#ifndef SERVO_H_
#define SERVO_H_

#include "pwm.h"
#include <stdint.h>

/* ?? Definiciones temporales SG90: 0.5 ms ~ 2.5 ms en periodo de 20 ms ?? */
#define SERVO_MIN_US     500   /* 0.5 ms  ?  0°  */
#define SERVO_MAX_US    2500   /* 2.5 ms  ? 180° */
#define SERVO_CENTER_US 1500   /* 1.5 ms  ?  90° */

/* Rango angular */
#define SERVO_ANGLE_MIN    0
#define SERVO_ANGLE_MAX  180

/*
 * Máximo de servos simultáneos.
 * El ATmega328P tiene 3 timers × 2 canales = 6 salidas PWM físicas.
 * Usar 16 desperdiciaría ~112 bytes de RAM (solo hay 2 KB en el Nano).
 */
#define SERVO_MAX_SERVOS   6

/* ?? Estructura de un servo ??????????????????????????????????????????????? */
typedef struct {
    pwm_timer_t   timer;
    pwm_channel_t channel;
    uint8_t       pin;       /* informativo; usado para configurar DDR externo */
    uint16_t      pulse_us;  /* último pulso enviado en µs                     */
    uint8_t       in_use;    /* 1 = slot ocupado, 0 = libre                    */
} servo_t;

/* ?? API pública ??????????????????????????????????????????????????????????? */

/* Inicializa el sistema (limpia tabla; no toca timers todavía) */
void servo_init(void);

/* Registra un servo en un canal PWM y configura su timer.
   Retorna índice 0..SERVO_MAX_SERVOS-1, o -1 si no hay espacio o falla. */
int servo_attach(pwm_timer_t timer, pwm_channel_t channel, uint8_t pin);

/* Libera el slot (no apaga el PWM; hazlo manualmente si lo necesitas) */
void servo_detach(int servo_idx);

/* Mueve el servo al ángulo indicado (0..180°).
   Retorna 0 = OK, -1 = error. */
int servo_move(int servo_idx, uint8_t angle);

/* Mueve el servo al pulso indicado en microsegundos (500..2500).
   Retorna 0 = OK, -1 = error. */
int servo_move_us(int servo_idx, uint16_t pulse_us);

/* Devuelve el último pulso enviado al servo (µs), 0 si inválido. */
uint16_t servo_get_us(int servo_idx);

/* Devuelve el ángulo aproximado actual (0..180), 0 si inválido. */
uint8_t servo_get_angle(int servo_idx);

#endif /* SERVO_H_ */