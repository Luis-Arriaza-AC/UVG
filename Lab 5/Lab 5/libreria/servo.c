/* servo.c */
#include "servo.h"
#include "pwm.h"

void servo_init(void)
{
	pwm_config_t cfg = {
		.timer     = PWM_TIMER2,
		.channel   = PWM_CHANNEL_B,     // OC2B ? pin D3
		.mode      = PWM_MODE_FAST,      // Fast PWM ? T = 16.384 ms
		.prescaler = PWM2_PRESCALER_1024,
		.polarity  = PWM_NON_INVERTED,
		.duty      = SERVO_OCR_MID,      // posición inicial: 90°
		.top       = 0                   // TOP = 0xFF (255) implícito en 8-bit
	};
	pwm_init(&cfg);
}

void servo_set_angle(uint8_t degrees)
{
	if (degrees > 180) degrees = 180;

	// Mapea 0-180° ? SERVO_OCR_MIN-SERVO_OCR_MAX
	uint8_t ocr = (uint8_t)(SERVO_OCR_MIN +
	((uint16_t)degrees * (SERVO_OCR_MAX - SERVO_OCR_MIN)) / 180);

	pwm_set_duty(PWM_TIMER2, PWM_CHANNEL_B, ocr);
}

void servo_set_raw(uint8_t ocr_val)
{
	pwm_set_duty(PWM_TIMER2, PWM_CHANNEL_B, ocr_val);
}