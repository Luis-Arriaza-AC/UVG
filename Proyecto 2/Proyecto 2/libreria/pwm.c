/*
 * pmw.c
 *
 * Created: 17/04/2026 13:17:39
 *  Author: luisz
 */ 

/* pwm.c */
#include "pwm.h"
#include <avr/io.h>

static void pwm_set_pin_output(pwm_timer_t timer, pwm_channel_t channel)
{
    if (timer == PWM_TIMER0) {
        if (channel == PWM_CHANNEL_A) DDRD |= (1 << PD6);
        else                          DDRD |= (1 << PD5);
    } else if (timer == PWM_TIMER1) {
        if (channel == PWM_CHANNEL_A) DDRB |= (1 << PB1);
        else                          DDRB |= (1 << PB2);
    } else {
        if (channel == PWM_CHANNEL_A) DDRB |= (1 << PB3);
        else                          DDRD |= (1 << PD3);
    }
}

static uint8_t pwm_com_bits_8bit(pwm_channel_t ch, pwm_polarity_t pol)
{
    uint8_t val = (pol == PWM_NON_INVERTED) ? 0x02 : 0x03;
    return (ch == PWM_CHANNEL_A) ? (val << COM0A0) : (val << COM0B0);
}

static uint8_t pwm_cs_bits_01(uint16_t prescaler)
{
    switch (prescaler) {
        case 1:    return (1 << CS00);
        case 8:    return (1 << CS01);
        case 64:   return (1 << CS01) | (1 << CS00);
        case 256:  return (1 << CS02);
        case 1024: return (1 << CS02) | (1 << CS00);
        default:   return (1 << CS01) | (1 << CS00);
    }
}

static uint8_t pwm_cs_bits_2(uint16_t prescaler)
{
    switch (prescaler) {
        case 1:    return (1 << CS20);
        case 8:    return (1 << CS21);
        case 32:   return (1 << CS21) | (1 << CS20);
        case 64:   return (1 << CS22);
        case 128:  return (1 << CS22) | (1 << CS20);
        case 256:  return (1 << CS22) | (1 << CS21);
        case 1024: return (1 << CS22) | (1 << CS21) | (1 << CS20);
        default:   return (1 << CS22);
    }
}

static void pwm_setup_timer0(const pwm_config_t *cfg)
{
    TCCR0A = 0; TCCR0B = 0;
    if (cfg->mode == PWM_MODE_FAST)
        TCCR0A |= (1 << WGM01) | (1 << WGM00);
    else
        TCCR0A |= (1 << WGM00);
    TCCR0A |= pwm_com_bits_8bit(cfg->channel, cfg->polarity);
    if (cfg->channel == PWM_CHANNEL_A) OCR0A = (uint8_t)cfg->duty;
    else                               OCR0B = (uint8_t)cfg->duty;
    TCCR0B |= pwm_cs_bits_01(cfg->prescaler);
}

static void pwm_setup_timer1(const pwm_config_t *cfg)
{
    TCCR1A = 0; TCCR1B = 0;
    uint16_t top = (cfg->top > 0) ? cfg->top : 0xFFFF;
    ICR1 = top;
    if (cfg->mode == PWM_MODE_FAST) {
        TCCR1A |= (1 << WGM11);
        TCCR1B |= (1 << WGM13) | (1 << WGM12);
    } else if (cfg->mode == PWM_MODE_PHASE_CORRECT) {
        TCCR1A |= (1 << WGM11);
        TCCR1B |= (1 << WGM13);
    } else {
        TCCR1B |= (1 << WGM13);
    }
    uint8_t com = (cfg->polarity == PWM_NON_INVERTED) ? 0x02 : 0x03;
    if (cfg->channel == PWM_CHANNEL_A) { TCCR1A |= (com << COM1A0); OCR1A = cfg->duty; }
    else                               { TCCR1A |= (com << COM1B0); OCR1B = cfg->duty; }
    TCCR1B |= pwm_cs_bits_01(cfg->prescaler);
}

static void pwm_setup_timer2(const pwm_config_t *cfg)
{
    TCCR2A = 0; TCCR2B = 0;
    if (cfg->mode == PWM_MODE_FAST)
        TCCR2A |= (1 << WGM21) | (1 << WGM20);
    else
        TCCR2A |= (1 << WGM20);
    uint8_t com = (cfg->polarity == PWM_NON_INVERTED) ? 0x02 : 0x03;
    if (cfg->channel == PWM_CHANNEL_A) { TCCR2A |= (com << COM2A0); OCR2A = (uint8_t)cfg->duty; }
    else                               { TCCR2A |= (com << COM2B0); OCR2B = (uint8_t)cfg->duty; }
    TCCR2B |= pwm_cs_bits_2(cfg->prescaler);
}

void pwm_init(const pwm_config_t *cfg)
{
    pwm_set_pin_output(cfg->timer, cfg->channel);
    switch (cfg->timer) {
        case PWM_TIMER0: pwm_setup_timer0(cfg); break;
        case PWM_TIMER1: pwm_setup_timer1(cfg); break;
        case PWM_TIMER2: pwm_setup_timer2(cfg); break;
    }
}

void pwm_set_duty(pwm_timer_t timer, pwm_channel_t channel, uint16_t duty)
{
    if (timer == PWM_TIMER0) {
        if (channel == PWM_CHANNEL_A) OCR0A = (uint8_t)duty;
        else                          OCR0B = (uint8_t)duty;
    } else if (timer == PWM_TIMER1) {
        if (channel == PWM_CHANNEL_A) OCR1A = duty;
        else                          OCR1B = duty;
    } else {
        if (channel == PWM_CHANNEL_A) OCR2A = (uint8_t)duty;
        else                          OCR2B = (uint8_t)duty;
    }
}

void pwm_stop(pwm_timer_t timer)
{
    switch (timer) {
        case PWM_TIMER0: TCCR0B &= ~((1<<CS02)|(1<<CS01)|(1<<CS00)); break;
        case PWM_TIMER1: TCCR1B &= ~((1<<CS12)|(1<<CS11)|(1<<CS10)); break;
        case PWM_TIMER2: TCCR2B &= ~((1<<CS22)|(1<<CS21)|(1<<CS20)); break;
    }
}