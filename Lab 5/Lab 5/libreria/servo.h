/* servo.h */
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

// Límites calibrados para SG90 con Timer2 Fast PWM, prescaler 1024
// Tick = 64 µs
// Pulso 0.5 ms ?  OCR =  8  ?   0°
// Pulso 1.5 ms ?  OCR = 23  ?  90°
// Pulso 2.5 ms ?  OCR = 39  ? 180°
#define SERVO_OCR_MIN  8
#define SERVO_OCR_MID  22
#define SERVO_OCR_MAX  39

// Si el servo no llega al extremo:
//   no alcanza   0° ? baja SERVO_OCR_MIN (mínimo 4)
//   no alcanza 180° ? sube SERVO_OCR_MAX (máximo 44, no pasar o se fuerza)

/**
 * servo_init() — configura Timer2 / OC2B (pin D3) para el SG90.
 * Llama esto una sola vez en setup().
 */
void servo_init(void);

/**
 * servo_set_angle() — mueve el servo a un ángulo entre 0 y 180 grados.
 */
void servo_set_angle(uint8_t degrees);

/**
 * servo_set_raw() — escribe directamente el valor OCR (SERVO_OCR_MIN a SERVO_OCR_MAX).
 * Útil para calibración fina.
 */
void servo_set_raw(uint8_t ocr_val);

#endif /* SERVO_H */