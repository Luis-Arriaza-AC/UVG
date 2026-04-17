/*
 * pmw.h
 *
 * Created: 17/04/2026 13:16:17
 *  Author: luisz
 */ 


/* pwm.h */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>


typedef enum {
    PWM_TIMER0,
    PWM_TIMER1,
    PWM_TIMER2
} pwm_timer_t;

typedef enum {
    PWM_CHANNEL_A,
    PWM_CHANNEL_B
} pwm_channel_t;

typedef enum {
    PWM_MODE_FAST,
    PWM_MODE_PHASE_CORRECT,
    PWM_MODE_PHASE_FREQ_CORRECT
} pwm_mode_t;

typedef enum {
    PWM_PRESCALER_1    = 1,
    PWM_PRESCALER_8    = 8,
    PWM_PRESCALER_64   = 64,
    PWM_PRESCALER_256  = 256,
    PWM_PRESCALER_1024 = 1024
} pwm_prescaler_t;

typedef enum {
    PWM2_PRESCALER_1    = 1,
    PWM2_PRESCALER_8    = 8,
    PWW2_PRESCALER_32   = 32,
    PWM2_PRESCALER_64   = 64,
    PWM2_PRESCALER_128  = 128,
    PWM2_PRESCALER_256  = 256,
    PWM2_PRESCALER_1024 = 1024
} pwm2_prescaler_t;

typedef enum {
    PWM_NON_INVERTED,
    PWM_INVERTED
} pwm_polarity_t;

typedef struct {
    pwm_timer_t    timer;
    pwm_channel_t  channel;
    pwm_mode_t     mode;
    uint16_t       prescaler;
    pwm_polarity_t polarity;
    uint16_t       duty;
    uint16_t       top;
} pwm_config_t;


void pwm_init    (const pwm_config_t *cfg);
void pwm_set_duty(pwm_timer_t timer, pwm_channel_t channel, uint16_t duty);
void pwm_stop    (pwm_timer_t timer);

#endif /* PWM_H */