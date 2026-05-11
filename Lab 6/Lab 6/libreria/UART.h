/*
 * UART.h
 *
 * Created: 
 * Author: 
 * Description: Librería UART para ATMega328P
 */

#ifndef UART_H_
#define UART_H_

void initUART(void);
void writeChar(char caracter);
void writeString(char* string);

#endif /* UART_H_ *//* servo.c */
#include "servo.h"
#include "pwm.h"
#include <stdint.h>

void servo_init_all(void)
{
    /*
     * Timer1 — Servos 0 y 1 (D9 y D10)
     * Fast PWM, TOP = ICR1 = 2499, prescaler 64 ? T = 20 ms exactos
     * Se inicializa con Channel A primero; Channel B comparte el mismo timer,
     * así que sólo ajustamos OCR1B después sin reconfigurar el timer entero.
     */
    pwm_config_t cfg1 = {
        .timer     = PWM_TIMER1,
        .channel   = PWM_CHANNEL_A,       /* D9 – OC1A */
        .mode      = PWM_MODE_FAST,
        .prescaler = PWM_PRESCALER_64,
        .polarity  = PWM_NON_INVERTED,
        .duty      = SERVO_T1_MID,        /* posición inicial 45° */
        .top       = SERVO_T1_TOP         /* ICR1 = 2499 */
    };
    pwm_init(&cfg1);

    /* Activar canal B del mismo Timer1 (D10 – OC1B) */
    cfg1.channel = PWM_CHANNEL_B;
    cfg1.duty    = SERVO_T1_MID;
    pwm_init(&cfg1);

    /*
     * Timer2 — Servos 2 y 3 (D11 y D3)
     * Fast PWM 8-bit, prescaler 1024 ? T ? 16.384 ms
     */
    pwm_config_t cfg2 = {
        .timer     = PWM_TIMER2,
        .channel   = PWM_CHANNEL_A,       /* D11 – OC2A */
        .mode      = PWM_MODE_FAST,
        .prescaler = PWM2_PRESCALER_1024,
        .polarity  = PWM_NON_INVERTED,
        .duty      = SERVO_T2_MID,        /* posición inicial 45° */
        .top       = 0                    /* TOP implícito = 0xFF */
    };
    pwm_init(&cfg2);

    cfg2.channel = PWM_CHANNEL_B;         /* D3 – OC2B */
    cfg2.duty    = SERVO_T2_MID;
    pwm_init(&cfg2);
}

void servo_set_angle(servo_id_t id, uint8_t degrees)
{
    /* Limitar a 90° (servo especificado) */
    if (degrees > 90) degrees = 90;

    switch (id) {
        case SERVO_0:
        case SERVO_1: {
            /* Timer1: mapeo 0–90° ? T1_MIN–T1_MID+?
             * Usamos la mitad del rango completo para 90°:
             *   0°  ? SERVO_T1_MIN (125)
             *   90° ? punto medio entre MIN y MAX = (125+625)/2 = 375 = T1_MID
             */
            uint16_t ocr = (uint16_t)(SERVO_T1_MIN +
                ((uint32_t)degrees * (SERVO_T1_MID - SERVO_T1_MIN)) / 90);
            pwm_set_duty(
                PWM_TIMER1,
                (id == SERVO_0) ? PWM_CHANNEL_A : PWM_CHANNEL_B,
                ocr
            );
            break;
        }
        case SERVO_2:
        case SERVO_3: {
            /* Timer2: mapeo 0–90° ? T2_MIN–T2_MID */
            uint8_t ocr = (uint8_t)(SERVO_T2_MIN +
                ((uint16_t)degrees * (SERVO_T2_MID - SERVO_T2_MIN)) / 90);
            pwm_set_duty(
                PWM_TIMER2,
                (id == SERVO_2) ? PWM_CHANNEL_A : PWM_CHANNEL_B,
                ocr
            );
            break;
        }
        default:
            break;
    }
}

void servo_set_raw(servo_id_t id, uint16_t ocr_val)
{
    switch (id) {
        case SERVO_0:
            pwm_set_duty(PWM_TIMER1, PWM_CHANNEL_A, ocr_val); break;
        case SERVO_1:
            pwm_set_duty(PWM_TIMER1, PWM_CHANNEL_B, ocr_val); break;
        case SERVO_2:
            pwm_set_duty(PWM_TIMER2, PWM_CHANNEL_A, (uint8_t)ocr_val); break;
        case SERVO_3:
            pwm_set_duty(PWM_TIMER2, PWM_CHANNEL_B, (uint8_t)ocr_val); break;
        default:
            break;
    }
}