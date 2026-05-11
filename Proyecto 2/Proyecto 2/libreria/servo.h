/* servo.h */
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include "pwm.h"

/*
 * Identificadores de servo. Cada uno mapea a un canal PWM concreto:
 *   SERVO_0 ? Timer1 Channel A ? OC1A ? D9
 *   SERVO_1 ? Timer1 Channel B ? OC1B ? D10
 *   SERVO_2 ? Timer2 Channel A ? OC2A ? D11
 *   SERVO_3 ? Timer2 Channel B ? OC2B ? D3
 *
 * Timer1: Fast PWM con ICR1 = 2499 y prescaler 64
 *   F_CPU = 16 MHz ? tick = 4 µs ? TOP = 2499 ? T = 10 000 µs ? 20 ms ?
 *   Pulso 0.5 ms ?  OCR = 125
 *   Pulso 1.5 ms ?  OCR = 375
 *   Pulso 2.5 ms ?  OCR = 625
 *
 * Timer2: Fast PWM 8-bit, prescaler 1024
 *   tick = 64 µs ? TOP = 255 ? T = 16.384 ms (aceptable para SG90)
 *   Pulso 0.5 ms ?  OCR = 8
 *   Pulso 1.5 ms ?  OCR = 23
 *   Pulso 2.5 ms ?  OCR = 39
 */
typedef enum {
    SERVO_0 = 0,   /* D9  – OC1A – Timer1 Ch.A */
    SERVO_1 = 1,   /* D10 – OC1B – Timer1 Ch.B */
    SERVO_2 = 2,   /* D11 – OC2A – Timer2 Ch.A */
    SERVO_3 = 3,   /* D3  – OC2B – Timer2 Ch.B */
    SERVO_COUNT
} servo_id_t;

/* Límites Timer1 (16-bit, período 20 ms) */
#define SERVO_T1_MIN   125u
#define SERVO_T1_MID   375u
#define SERVO_T1_MAX   625u
#define SERVO_T1_TOP  2499u

/* Límites Timer2 (8-bit, período 16.384 ms) */
#define SERVO_T2_MIN    8u
#define SERVO_T2_MID   23u
#define SERVO_T2_MAX   39u

/**
 * servo_init_all() — configura Timer1 y Timer2 para los 4 servos.
 * Llama una sola vez en la inicialización.
 */
void servo_init_all(void);

/**
 * servo_set_angle(id, degrees) — mueve el servo indicado a 0–90°.
 * El rango se limita a 90° según la especificación (servo de 90°).
 */
void servo_set_angle(servo_id_t id, uint8_t degrees);

/**
 * servo_set_raw(id, ocr_val) — escribe directamente el valor OCR.
 * Para Timer1: rango 125–625. Para Timer2: rango 8–39.
 */
void servo_set_raw(servo_id_t id, uint16_t ocr_val);

#endif /* SERVO_H */