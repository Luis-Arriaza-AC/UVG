/*
 * pwm.h
 *
 * Created: 15/05/2026 15:21:35
 *  Author: luisz
 */ 


#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>
#include <avr/io.h>

/* Tipos públicos */
typedef enum {
    PWM_TIMER0, /* pins 5,6 (8-bit) */
    PWM_TIMER1, /* pins 9,10 (16-bit) */
    PWM_TIMER2  /* pins 3,11 (8-bit) */
} pwm_timer_t;

typedef enum {
    PWM_CHANNEL_A,
    PWM_CHANNEL_B
} pwm_channel_t;

typedef enum {
    PWM_MODE_FAST,          /* Fast PWM */
    PWM_MODE_PHASE_CORRECT  /* Phase-correct PWM */
} pwm_mode_t;

/* Prescalers (representativos); not all values aplican a todos timers */
typedef enum {
	PWM_PRESCALE_NONE = 0,
	PWM_PRESCALE_1,
	PWM_PRESCALE_8,
	PWM_PRESCALE_32,    // ? solo Timer2
	PWM_PRESCALE_64,
	PWM_PRESCALE_128,   // ? solo Timer2
	PWM_PRESCALE_256,
	PWM_PRESCALE_1024
} pwm_prescale_t;

/* Inicializa internamente (no cambia timers) — opcional */
void pwm_init(void);

/* Configura el modo y prescaler de un timer sin tocar los canales OCR existentes
   Si quieres cambiar modo o prescaler de un timer, llama esta función.
   mode: PWM_MODE_FAST o PWM_MODE_PHASE_CORRECT
   top: para Timer1 puede usarse 8/9/10-bit o ICR1/OCR1A; si top == 0 usa el top por defecto del modo seleccionado.
   Returns 0 on success, non-zero on invalid args.
*/
int pwm_config_timer(pwm_timer_t timer, pwm_mode_t mode, uint32_t top, pwm_prescale_t prescaler);

/* Habilita salida PWM en un pin (timer+canal) y fija duty (0..max) sin modificar el otro canal del timer.
   duty: valor entre 0..255 para timers 8-bit, 0..65535 para timer1 (16-bit).
   Returns 0 on success.
*/
int pwm_set_channel(pwm_timer_t timer, pwm_channel_t channel, uint32_t duty);

/* Deshabilita la salida PWM del canal, dejando el timer y el otro canal intactos.
   Cuando se deshabilita, el pin queda como entrada (tri-state) o puedes configurar como digital output manualmente.
*/
int pwm_disable_channel(pwm_timer_t timer, pwm_channel_t channel);

/* Consulta el valor de duty actual (0..max). */
uint32_t pwm_get_duty(pwm_timer_t timer, pwm_channel_t channel);

#endif /* PWM_AVR_H */