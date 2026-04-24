/*
 * Lab4.c
 *
 * Created: 10/4/26
 * Author: Luis Arriaza
 * Description: Control de dos servos SG90
 *              Servo 1 ? Timer1 / D9  / pot en A6
 *              Servo 2 ? Timer2 / D3  / pot en A7
 */
/****************************************/
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "libreria/pwm.h"
#include "libreria/servo.h"

//Valores servo timer 1
#define SERVO_TOP  39999
#define SERVO_MIN  1000
#define SERVO_MAX  5000

/****************************************/
void     setup(void);
uint16_t leer_adc(uint8_t canal);
uint16_t adc_a_servo(uint16_t adc_val);

/****************************************/
int main(void)
{
    setup();

    while (1)
    {
        //Servo 1: Timer1, pot en A6
        uint16_t adc_a6 = leer_adc(6);
        uint16_t duty   = adc_a_servo(adc_a6);
        pwm_set_duty(PWM_TIMER1, PWM_CHANNEL_A, duty);

        // --- Servo 2: Timer2, pot en A7 (canal 7) ---
        uint16_t adc_a7 = leer_adc(7);
        uint8_t  angulo = (uint8_t)((uint32_t)adc_a7 * 180 / 1023);
        servo_set_angle(angulo);

        _delay_ms(20);
    }
}

/****************************************/
void setup(void)
{
    // ADC: referencia AVCC, prescaler 128
    // El canal se selecciona en leer_adc() antes de cada conversión
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Servo 1 en D9 (OC1A), Timer1 16-bit, 50 Hz — sin cambios
    pwm_config_t servo1 = {
        .timer     = PWM_TIMER1,
        .channel   = PWM_CHANNEL_A,
        .mode      = PWM_MODE_FAST,
        .prescaler = PWM_PRESCALER_8,
        .polarity  = PWM_NON_INVERTED,
        .duty      = SERVO_MIN,
        .top       = SERVO_TOP
    };
    pwm_init(&servo1);

    // Servo 2 en D3 (OC2B), Timer2, 16.384 ms
    servo_init();
}

/**
 * leer_adc() — selecciona el canal antes de convertir.
 * canal: 0-7 corresponde a A0-A7.
 */
uint16_t leer_adc(uint8_t canal)
{
    // REFS0 = AVCC, los 3 bits bajos de MUX seleccionan el canal
    ADMUX  = (1 << REFS0) | (canal & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

uint16_t adc_a_servo(uint16_t adc_val)
{
    return (uint16_t)(SERVO_MIN +
           ((uint32_t)adc_val * (SERVO_MAX - SERVO_MIN)) / 1023);
}