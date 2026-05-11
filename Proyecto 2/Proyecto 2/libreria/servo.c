/* servo.c */
#include "servo.h"
#include "pwm.h"
#include <avr/io.h>
#include <stdint.h>

/*
 * Timer1 tiene el bug de doble-reset cuando se llama pwm_init() dos veces:
 * la segunda llamada ejecuta "TCCR1A = 0; TCCR1B = 0" borrando lo que
 * configuró la primera. Se resuelve configurando ambos canales manualmente
 * en una sola función, sin pasar por pwm_init() para el canal B.
 *
 * Timer1 Fast PWM, TOP = ICR1:
 *   WGM13:10 = 1110  ?  TCCR1B |= WGM13|WGM12 / TCCR1A |= WGM11
 *   Prescaler 64     ?  CS11|CS10
 *   F_CPU=16MHz, tick=4µs, ICR1=2499 ? T=10 000µs = 20 ms ?
 *
 *   OCR para ángulos (0–90°):
 *     0°  ? 0.5 ms ? 125 ticks
 *     45° ? 1.0 ms ? 250 ticks  (usamos como posición inicial)
 *     90° ? 1.5 ms ? 375 ticks
 */
static void servo_init_timer1_dual(void)
{
    /* Configurar D9 (OC1A) y D10 (OC1B) como salidas */
    DDRB |= (1 << PB1) | (1 << PB2);

    /* Resetear registros */
    TCCR1A = 0;
    TCCR1B = 0;

    /* TOP = ICR1 = 2499 */
    ICR1 = SERVO_T1_TOP;

    /* Posición inicial: 45° en ambos canales */
    OCR1A = SERVO_T1_MID;
    OCR1B = SERVO_T1_MID;

    /*
     * TCCR1A:
     *   COM1A1=1, COM1A0=0 ? OC1A activo no-invertido
     *   COM1B1=1, COM1B0=0 ? OC1B activo no-invertido
     *   WGM11=1, WGM10=0   ? parte baja del modo Fast PWM TOP=ICR1
     */
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);

    /*
     * TCCR1B:
     *   WGM13=1, WGM12=1   ? parte alta del modo Fast PWM TOP=ICR1
     *   CS11=1,  CS10=1    ? prescaler 64
     */
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
}

/*
 * Timer2 no tiene el problema de doble-init porque pwm_init() con canal A
 * y luego canal B sólo suma bits COM2x al TCCR2A ya configurado.
 * Sin embargo, para ser consistentes lo inicializamos también en un bloque.
 *
 * Timer2 Fast PWM 8-bit, prescaler 1024:
 *   tick = 64 µs, TOP = 255 ? T = 16.384 ms
 *   0°  ? 0.5 ms ? OCR =  8
 *   45° ? 1.0 ms ? OCR = ~15  (posición inicial)
 *   90° ? 1.5 ms ? OCR = 23
 */
static void servo_init_timer2_dual(void)
{
    /* Configurar D11 (OC2A) y D3 (OC2B) como salidas */
    DDRB |= (1 << PB3);   /* D11 = OC2A */
    DDRD |= (1 << PD3);   /* D3  = OC2B */

    TCCR2A = 0;
    TCCR2B = 0;

    OCR2A = SERVO_T2_MID;
    OCR2B = SERVO_T2_MID;

    /*
     * TCCR2A:
     *   COM2A1=1 ? OC2A no-invertido
     *   COM2B1=1 ? OC2B no-invertido
     *   WGM21=1, WGM20=1 ? Fast PWM TOP=0xFF
     */
    TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);

    /*
     * TCCR2B:
     *   CS22=1, CS21=1, CS20=1 ? prescaler 1024
     */
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
}

void servo_init_all(void)
{
    servo_init_timer1_dual();
    servo_init_timer2_dual();
}

void servo_set_angle(servo_id_t id, uint8_t degrees)
{
    if (degrees > 90) degrees = 90;

    switch (id) {
        case SERVO_0:
        case SERVO_1: {
            /* 0–90° ? SERVO_T1_MIN(125) – SERVO_T1_MID(375) */
            uint16_t ocr = (uint16_t)(SERVO_T1_MIN +
                ((uint32_t)degrees * (SERVO_T1_MID - SERVO_T1_MIN)) / 90);
            if (id == SERVO_0) OCR1A = ocr;
            else               OCR1B = ocr;
            break;
        }
        case SERVO_2:
        case SERVO_3: {
            /* 0–90° ? SERVO_T2_MIN(8) – SERVO_T2_MID(23) */
            uint8_t ocr = (uint8_t)(SERVO_T2_MIN +
                ((uint16_t)degrees * (SERVO_T2_MID - SERVO_T2_MIN)) / 90);
            if (id == SERVO_2) OCR2A = ocr;
            else               OCR2B = ocr;
            break;
        }
        default: break;
    }
}

void servo_set_raw(servo_id_t id, uint16_t ocr_val)
{
    switch (id) {
        case SERVO_0: OCR1A = ocr_val;         break;
        case SERVO_1: OCR1B = ocr_val;         break;
        case SERVO_2: OCR2A = (uint8_t)ocr_val; break;
        case SERVO_3: OCR2B = (uint8_t)ocr_val; break;
        default: break;
    }
}