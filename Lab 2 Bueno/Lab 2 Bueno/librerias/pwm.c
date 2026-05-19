/*
 * pwm.c
 *
 * Created: 15/05/2026 15:20:58
 *  Author: luisz
 */ 
#include "pwm.h"
#include <avr/interrupt.h>

/* ??? Helpers de prescaler por timer ????????????????????????????????????????? */

static inline uint8_t prescale_cs_timer01(pwm_prescale_t p) {
    /* Timer0 y Timer1 comparten la misma tabla CS */
    switch (p) {
        case PWM_PRESCALE_1:    return (1<<CS00);
        case PWM_PRESCALE_8:    return (1<<CS01);
        case PWM_PRESCALE_64:   return (1<<CS01)|(1<<CS00);
        case PWM_PRESCALE_256:  return (1<<CS02);
        case PWM_PRESCALE_1024: return (1<<CS02)|(1<<CS00);
        default:                return 0;
    }
}

static inline uint8_t prescale_cs_timer2(pwm_prescale_t p) {
	switch (p) {
		case PWM_PRESCALE_1:    return (1<<CS20);                          // 1
		case PWM_PRESCALE_8:    return (1<<CS21);                          // 8
		case PWM_PRESCALE_32:   return (1<<CS21)|(1<<CS20);               // 32  ? exclusivo Timer2
		case PWM_PRESCALE_64:   return (1<<CS22);                          // 64
		case PWM_PRESCALE_128:  return (1<<CS22)|(1<<CS20);               // 128 ? exclusivo Timer2
		case PWM_PRESCALE_256:  return (1<<CS22)|(1<<CS21);               // 256
		case PWM_PRESCALE_1024: return (1<<CS22)|(1<<CS21)|(1<<CS20);     // 1024
		default:                return 0;
	}
}

/* ??? Timer0 (8-bit) ??????????????????????????????????????????????????????? */
static int configure_timer0(pwm_mode_t mode, uint32_t top, pwm_prescale_t prescaler) {
    uint8_t a = TCCR0A;
    uint8_t b = TCCR0B;

    a &= ~((1<<WGM01)|(1<<WGM00));
    b &= ~((1<<WGM02)|(1<<CS02)|(1<<CS01)|(1<<CS00));

    if (mode == PWM_MODE_FAST) {
        /* Fast PWM 8-bit: WGM02:0 = 3 ? WGM01=1, WGM00=1 */
        a |= (1<<WGM01)|(1<<WGM00);
        /* WGM02=0 ya limpiado */
    } else {
        /* Phase-correct 8-bit: WGM02:0 = 1 ? WGM00=1 */
        a |= (1<<WGM00);
    }

    b |= prescale_cs_timer01(prescaler);
    TCCR0A = a;
    TCCR0B = b;
    return 0;
}

/* ??? Timer1 (16-bit) ????????????????????????????????????????????????????? */
static int configure_timer1(pwm_mode_t mode, uint32_t top, pwm_prescale_t prescaler) {
    uint8_t a = TCCR1A;
    uint8_t b = TCCR1B;

    a &= ~((1<<WGM11)|(1<<WGM10));
    b &= ~((1<<WGM13)|(1<<WGM12)|(1<<CS12)|(1<<CS11)|(1<<CS10));

    if (mode == PWM_MODE_FAST) {
        if (top > 0) {
            /* Fast PWM, TOP = ICR1: WGM13:0 = 14 (0b1110)
               WGM13=1, WGM12=1, WGM11=1, WGM10=0 */
            a |= (1<<WGM11);
            b |= (1<<WGM13)|(1<<WGM12);
            ICR1 = (uint16_t)top;
        } else {
            /* Fast PWM 10-bit: WGM13:0 = 7 (0b0111)
               WGM12=1, WGM11=1, WGM10=1 */
            a |= (1<<WGM11)|(1<<WGM10);
            b |= (1<<WGM12);
        }
    } else {
        if (top > 0) {
            /* Phase-correct, TOP = ICR1: WGM13:0 = 10 (0b1010)
               WGM13=1, WGM12=0, WGM11=1, WGM10=0 */
            a |= (1<<WGM11);          /* ? FALTABA en versión original */
            b |= (1<<WGM13);
            ICR1 = (uint16_t)top;
        } else {
            /* Phase-correct 10-bit: WGM13:0 = 3 (0b0011)
               WGM11=1, WGM10=1 */
            a |= (1<<WGM11)|(1<<WGM10);  /* ? corregido; original solo ponía WGM10 */
        }
    }

    b |= prescale_cs_timer01(prescaler);
    TCCR1A = a;
    TCCR1B = b;
    return 0;
}

/* ??? Timer2 (8-bit) ??????????????????????????????????????????????????????? */
static int configure_timer2(pwm_mode_t mode, uint32_t top, pwm_prescale_t prescaler) {
    uint8_t a = TCCR2A;
    uint8_t b = TCCR2B;

    a &= ~((1<<WGM21)|(1<<WGM20));
    b &= ~((1<<WGM22)|(1<<CS22)|(1<<CS21)|(1<<CS20));

    if (mode == PWM_MODE_FAST) {
        /* Fast PWM 8-bit: WGM22:0 = 3 ? WGM21=1, WGM20=1 */
        a |= (1<<WGM21)|(1<<WGM20);
    } else {
        /* Phase-correct 8-bit: WGM22:0 = 1 ? WGM20=1 */
        a |= (1<<WGM20);
    }

    b |= prescale_cs_timer2(prescaler);   /* ? tabla correcta de Timer2 */
    TCCR2A = a;
    TCCR2B = b;
    return 0;
}

/* ??? API pública ??????????????????????????????????????????????????????????? */

void pwm_init(void) { /* reservado */ }

int pwm_config_timer(pwm_timer_t timer, pwm_mode_t mode,
                     uint32_t top, pwm_prescale_t prescaler) {
    switch (timer) {
        case PWM_TIMER0: return configure_timer0(mode, top, prescaler);
        case PWM_TIMER1: return configure_timer1(mode, top, prescaler);
        case PWM_TIMER2: return configure_timer2(mode, top, prescaler);
        default:         return -1;
    }
}

int pwm_set_channel(pwm_timer_t timer, pwm_channel_t channel, uint32_t duty) {
    switch (timer) {
        case PWM_TIMER0:
            if (channel == PWM_CHANNEL_A) {
                OCR0A = (uint8_t)duty;
                TCCR0A = (TCCR0A & ~((1<<COM0A1)|(1<<COM0A0))) | (1<<COM0A1);
            } else {
                OCR0B = (uint8_t)duty;
                TCCR0A = (TCCR0A & ~((1<<COM0B1)|(1<<COM0B0))) | (1<<COM0B1);
            }
            return 0;
        case PWM_TIMER1:
            if (channel == PWM_CHANNEL_A) {
                OCR1A = (uint16_t)duty;
                TCCR1A = (TCCR1A & ~((1<<COM1A1)|(1<<COM1A0))) | (1<<COM1A1);
            } else {
                OCR1B = (uint16_t)duty;
                TCCR1A = (TCCR1A & ~((1<<COM1B1)|(1<<COM1B0))) | (1<<COM1B1);
            }
            return 0;
        case PWM_TIMER2:
            if (channel == PWM_CHANNEL_A) {
                OCR2A = (uint8_t)duty;
                TCCR2A = (TCCR2A & ~((1<<COM2A1)|(1<<COM2A0))) | (1<<COM2A1);
            } else {
                OCR2B = (uint8_t)duty;
                TCCR2A = (TCCR2A & ~((1<<COM2B1)|(1<<COM2B0))) | (1<<COM2B1);
            }
            return 0;
        default:
            return -1;
    }
}

int pwm_disable_channel(pwm_timer_t timer, pwm_channel_t channel) {
    switch (timer) {
        case PWM_TIMER0:
            if (channel == PWM_CHANNEL_A) TCCR0A &= ~((1<<COM0A1)|(1<<COM0A0));
            else                           TCCR0A &= ~((1<<COM0B1)|(1<<COM0B0));
            return 0;
        case PWM_TIMER1:
            if (channel == PWM_CHANNEL_A) TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));
            else                           TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));
            return 0;
        case PWM_TIMER2:
            if (channel == PWM_CHANNEL_A) TCCR2A &= ~((1<<COM2A1)|(1<<COM2A0));
            else                           TCCR2A &= ~((1<<COM2B1)|(1<<COM2B0));
            return 0;
        default:
            return -1;
    }
}

uint32_t pwm_get_duty(pwm_timer_t timer, pwm_channel_t channel) {
    switch (timer) {
        case PWM_TIMER0: return (channel == PWM_CHANNEL_A) ? OCR0A : OCR0B;
        case PWM_TIMER1: return (channel == PWM_CHANNEL_A) ? OCR1A : OCR1B;
        case PWM_TIMER2: return (channel == PWM_CHANNEL_A) ? OCR2A : OCR2B;
        default:         return 0;
    }
}