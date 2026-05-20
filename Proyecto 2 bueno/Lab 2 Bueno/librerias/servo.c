/*
 * servo.c  –  corregido (Timer2 prescaler y OCR)
 */
#include "servo.h"
#include <stdint.h>
#include <avr/io.h>

static uint8_t timer_configured[3] = {0, 0, 0};
static servo_t servos[SERVO_MAX_SERVOS];
static int servo_count = 0;

void servo_init(void) {
    int i;
    for (i = 0; i < SERVO_MAX_SERVOS; i++) servos[i].in_use = 0;
    servo_count = 0;
    timer_configured[0] = 0;
    timer_configured[1] = 0;
    timer_configured[2] = 0;
}

static uint16_t servo_angle_to_us(uint8_t angle) {
    if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;
    if (angle < SERVO_ANGLE_MIN) angle = SERVO_ANGLE_MIN;
    return (uint16_t)((uint32_t)(angle - SERVO_ANGLE_MIN)
           * (SERVO_MAX_US - SERVO_MIN_US)
           / (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN)
           + SERVO_MIN_US);
}

static uint8_t servo_us_to_angle(uint16_t us) {
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;
    return (uint8_t)((uint32_t)(us - SERVO_MIN_US)
           * (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN)
           / (SERVO_MAX_US - SERVO_MIN_US)
           + SERVO_ANGLE_MIN);
}

/*
 * servo_us_to_ocr
 *
 * Timer1  prescaler 8,  ICR1=39999: tick=0.5µs  ? OCR = us * 2
 * Timer2  prescaler 1024, top=255:  tick=64µs    ? OCR = us / 64
 * Timer0  igual que Timer2 (no recomendado para servos)
 *
 * Nota: Timer2 con prescaler 1024 da ~61 Hz (periodo 16.384 ms).
 * Rango OCR: 500/64=7 (0°) a 2500/64=39 (180°) ? 32 pasos, suficiente
 * para movimiento visible aunque sin alta resolución.
 */
static uint32_t servo_us_to_ocr(servo_t *s, uint16_t us) {
    switch (s->timer) {
        case PWM_TIMER1:
            /* tick = 0.5 µs ? OCR = us * 2 */
            return (uint32_t)us * 2UL;

        case PWM_TIMER0:
        case PWM_TIMER2:
            /*
             * prescaler 1024, top 255
             * tick = 1024 / 16000000 = 64 µs
             * OCR  = us / 64
             * Máximo: 2500/64 = 39 < 255 ? no hay overflow
             */
            return (uint32_t)us / 64UL;

        default:
            return 0;
    }
}

/*
 * servo_configure_timer
 *
 * Timer1: Fast PWM, ICR1=39999, prescaler 8 ? exactamente 50 Hz
 *
 * Timer2: Fast PWM, top=255, prescaler 1024 ? ~61 Hz
 *   CS22|CS21|CS20 = 0b111 ? prescaler 1024 (corregido respecto a versión anterior)
 *
 * Se configura solo una vez por timer aunque haya 2 canales.
 */
static int servo_configure_timer(servo_t *s) {
    if (!s->in_use) return -1;

    uint8_t tidx = (uint8_t)s->timer;
    if (tidx > 2) return -1;
    if (timer_configured[tidx]) return 0;   /* ya configurado, no tocar */

    int ret;
    switch (s->timer) {
        case PWM_TIMER1:
            /* Fast PWM TOP=ICR1=39999, prescaler 8
             * f = 16MHz / (8 * 40000) = 50 Hz exacto ?               */
            ret = pwm_config_timer(PWM_TIMER1, PWM_MODE_FAST,
                                   39999, PWM_PRESCALE_8);
            break;

        case PWM_TIMER0:
        case PWM_TIMER2:
            /* Fast PWM 8-bit (top fijo 255), prescaler 1024
             * f = 16MHz / (1024 * 256) ? 61 Hz
             * CS correcto para Timer2: CS22|CS21|CS20 = 0b111 = 1024  */
            ret = pwm_config_timer(s->timer, PWM_MODE_FAST,
                                   255, PWM_PRESCALE_1024);
            break;

        default:
            return -1;
    }

    if (ret == 0) timer_configured[tidx] = 1;
    return ret;
}

int servo_attach(pwm_timer_t timer, pwm_channel_t channel, uint8_t pin) {
    int i;
    if (servo_count >= SERVO_MAX_SERVOS) return -1;
    for (i = 0; i < SERVO_MAX_SERVOS; i++) {
        if (!servos[i].in_use) {
            servos[i].timer    = timer;
            servos[i].channel  = channel;
            servos[i].pin      = pin;
            servos[i].pulse_us = SERVO_CENTER_US;
            servos[i].in_use   = 1;
            servo_count++;
            if (servo_configure_timer(&servos[i]) != 0) {
                servos[i].in_use = 0;
                servo_count--;
                return -1;
            }
            return i;
        }
    }
    return -1;
}

void servo_detach(int servo_idx) {
    if (servo_idx < 0 || servo_idx >= SERVO_MAX_SERVOS) return;
    if (!servos[servo_idx].in_use) return;
    servos[servo_idx].in_use = 0;
    servo_count--;
}

int servo_move(int servo_idx, uint8_t angle) {
    if (servo_idx < 0 || servo_idx >= SERVO_MAX_SERVOS) return -1;
    if (!servos[servo_idx].in_use) return -1;

    uint16_t us  = servo_angle_to_us(angle);
    uint32_t ocr = servo_us_to_ocr(&servos[servo_idx], us);

    if (pwm_set_channel(servos[servo_idx].timer,
                        servos[servo_idx].channel, ocr) != 0)
        return -1;

    servos[servo_idx].pulse_us = us;
    return 0;
}

int servo_move_us(int servo_idx, uint16_t pulse_us) {
    if (servo_idx < 0 || servo_idx >= SERVO_MAX_SERVOS) return -1;
    if (!servos[servo_idx].in_use) return -1;

    if (pulse_us < SERVO_MIN_US) pulse_us = SERVO_MIN_US;
    if (pulse_us > SERVO_MAX_US) pulse_us = SERVO_MAX_US;

    uint32_t ocr = servo_us_to_ocr(&servos[servo_idx], pulse_us);

    if (pwm_set_channel(servos[servo_idx].timer,
                        servos[servo_idx].channel, ocr) != 0)
        return -1;

    servos[servo_idx].pulse_us = pulse_us;
    return 0;
}

uint16_t servo_get_us(int servo_idx) {
    if (servo_idx < 0 || servo_idx >= SERVO_MAX_SERVOS) return 0;
    if (!servos[servo_idx].in_use) return 0;
    return servos[servo_idx].pulse_us;
}

uint8_t servo_get_angle(int servo_idx) {
    if (servo_idx < 0 || servo_idx >= SERVO_MAX_SERVOS) return 0;
    if (!servos[servo_idx].in_use) return 0;
    return servo_us_to_angle(servos[servo_idx].pulse_us);
}