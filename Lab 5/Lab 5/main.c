/*
 * Lab4.c
 *
 * Created: 10/4/26
 * Author: Luis Arriaza
 * Description: Control de servo SG90 con potenciómetro en A6
 */
/****************************************/
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "libreria/pwm.h"

// Ajusta estos dos valores hasta obtener 0° y 180° reales.
// Empieza con estos y mueve el pot a los extremos.
// Si no llega a 0°  ? baja SERVO_MIN (mínimo ~600)
// Si no llega a 180° ? sube SERVO_MAX (máximo ~5500, cuidado de no forzar el motor)
#define SERVO_TOP  39999
#define SERVO_MIN  1000
#define SERVO_MAX  5000

/****************************************/
void     setup(void);
uint16_t leer_adc(void);
uint16_t adc_a_servo(uint16_t adc_val);

/****************************************/
int main(void)
{
    setup();

    while (1)
    {
        uint16_t adc_raw = leer_adc();
        uint16_t duty    = adc_a_servo(adc_raw);
        pwm_set_duty(PWM_TIMER1, PWM_CHANNEL_A, duty);
        _delay_ms(20);
    }
}

/****************************************/
void setup(void)
{
    // ADC en PC6 (A6), referencia AVCC, prescaler 128
    ADMUX  = (1 << REFS0) | (1 << MUX2) | (1 << MUX1);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Servo en D9 (OC1A), Timer1 16-bit, 50 Hz
    pwm_config_t servo = {
        .timer     = PWM_TIMER1,
        .channel   = PWM_CHANNEL_A,
        .mode      = PWM_MODE_FAST,
        .prescaler = PWM_PRESCALER_8,
        .polarity  = PWM_NON_INVERTED,
        .duty      = SERVO_MIN,
        .top       = SERVO_TOP
    };
    pwm_init(&servo);
}

uint16_t leer_adc(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

uint16_t adc_a_servo(uint16_t adc_val)
{
    return (uint16_t)(SERVO_MIN +
           ((uint32_t)adc_val * (SERVO_MAX - SERVO_MIN)) / 1023);
}