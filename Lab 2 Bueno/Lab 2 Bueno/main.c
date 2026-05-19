/*
 * Lab 2 Bueno.c  – con modo 1 serial
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "librerias/pwm.h"
#include "librerias/servo.h"
#include "librerias/UART.h"

/* ?? Pines ?????????????????????????????????????????????????????????????? */
#define PIN_SERVO_D3   3   // PD3
#define PIN_BTN_UP     4   // PD4
#define PIN_BTN_DOWN   5   // PD5
#define PIN_LED_MODE0  6   // PD6
#define PIN_LED_MODE1  7   // PD7

#define PIN_SERVO_D9   1   // PB1
#define PIN_SERVO_D10  2   // PB2
#define PIN_SERVO_D11  3   // PB3

#define MODE_MIN 0
#define MODE_MAX 3

#define PCINT_BTN_UP   (1<<PIN_BTN_UP)
#define PCINT_BTN_DOWN (1<<PIN_BTN_DOWN)

/* ?? Prototipos ????????????????????????????????????????????????????????? */
void init_ports(void);
void init_pin_change_interrupts(void);
void update_mode_leds(uint8_t mode);

void mode0_control_with_pot(void);
void mode1_serial_control(void);       
void mode2_saved_positions(void);
void mode3_default_positions(void);

uint16_t read_adc(uint8_t channel);

/* ?? Globals ???????????????????????????????????????????????????????????? */
volatile uint8_t current_mode = 0;
static int8_t h_d3, h_d9, h_d10, h_d11;

/****************************************/
// Main
/****************************************/
int main(void) {
    init_ports();
    init_pin_change_interrupts();
    servo_init();
    initUART();                        /* ? inicializar UART */

    h_d3  = servo_attach(PWM_TIMER2, PWM_CHANNEL_B, PIN_SERVO_D3);
    h_d9  = servo_attach(PWM_TIMER1, PWM_CHANNEL_A, PIN_SERVO_D9);
    h_d10 = servo_attach(PWM_TIMER1, PWM_CHANNEL_B, PIN_SERVO_D10);
    h_d11 = servo_attach(PWM_TIMER2, PWM_CHANNEL_A, PIN_SERVO_D11);

    if (h_d3 < 0 || h_d9 < 0 || h_d10 < 0 || h_d11 < 0) {
        while (1) {
            PORTD |=  (1<<PIN_LED_MODE0)|(1<<PIN_LED_MODE1);
            _delay_ms(100);
            PORTD &= ~((1<<PIN_LED_MODE0)|(1<<PIN_LED_MODE1));
            _delay_ms(100);
        }
    }

    sei();
    update_mode_leds(current_mode);

    while (1) {
        cli();
        uint8_t mode_snapshot = current_mode;
        sei();

        switch (mode_snapshot) {
            case 0: mode0_control_with_pot();  break;
            case 1: mode1_serial_control();    break;
            case 2: mode2_fixed_positions();   break;
            case 3: mode3_alternating_sweep(); break;
            default:
                cli(); current_mode = MODE_MIN; sei();
                update_mode_leds(MODE_MIN);
                break;
        }
        _delay_ms(20);
    }
}

/****************************************/
// Subroutines
/****************************************/

void init_ports(void) {
    DDRD |=  (1<<PIN_SERVO_D3)|(1<<PIN_LED_MODE0)|(1<<PIN_LED_MODE1);
    DDRD &= ~((1<<PIN_BTN_UP)|(1<<PIN_BTN_DOWN));
    PORTD |=  (1<<PIN_BTN_UP)|(1<<PIN_BTN_DOWN);

    DDRB |= (1<<PIN_SERVO_D9)|(1<<PIN_SERVO_D10)|(1<<PIN_SERVO_D11);

    ADMUX  = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
}

void init_pin_change_interrupts(void) {
    PCICR  |= (1<<PCIE2);
    PCMSK2 |= (PCINT_BTN_UP | PCINT_BTN_DOWN);
}

void update_mode_leds(uint8_t mode) {
    if (mode & 0x01) PORTD |=  (1<<PIN_LED_MODE0);
    else             PORTD &= ~(1<<PIN_LED_MODE0);
    if (mode & 0x02) PORTD |=  (1<<PIN_LED_MODE1);
    else             PORTD &= ~(1<<PIN_LED_MODE1);
}

void mode0_control_with_pot(void) {
    uint16_t v0 = read_adc(0);
    uint16_t v1 = read_adc(1);
    uint16_t v2 = read_adc(2);
    uint16_t v3 = read_adc(3);

    servo_move(h_d3,  (uint8_t)((uint32_t)v0 * SERVO_ANGLE_MAX / 1023));
    servo_move(h_d9,  (uint8_t)((uint32_t)v1 * SERVO_ANGLE_MAX / 1023));
    servo_move(h_d10, (uint8_t)((uint32_t)v2 * SERVO_ANGLE_MAX / 1023));
    servo_move(h_d11, (uint8_t)((uint32_t)v3 * SERVO_ANGLE_MAX / 1023));
}

/*
 * mode1_serial_control
 *
 * Flujo:
 *   1. Muestra menú de selección de servo.
 *   2. Usuario elige 1-4 o 'q' para salir del modo.
 *   3. Pide ángulo (0-180).
 *   4. Mueve el servo y confirma.
 *   5. Vuelve al menú de selección.
 *   6. 'q' ? sale de la función (el loop principal revisará current_mode).
 *
 * IMPORTANTE: readChar() y readUInt8() son bloqueantes.
 * Si el usuario presiona un botón mientras está en este modo,
 * current_mode cambia en la ISR; al salir del bloqueo serial
 * el loop principal detecta el cambio y abandona el modo.
 */
void mode1_serial_control(void) {
	typedef enum { S_SHOW_MENU, S_WAIT_SERVO, S_ASK_ANGLE, S_WAIT_ANGLE } mode1_state_t;
	static mode1_state_t state = S_SHOW_MENU;

	/* Declarar todo al inicio — C99 con avr-gcc puede ser estricto en switch */
	int8_t  handle = -1;
	char   *nombre = "";
	char    opcion = 0;

	state = S_SHOW_MENU;
	writeString("\r\n=== MODO 1: Control Serial de Servos ===\r\n");
	writeString("Presiona un boton fisico para cambiar de modo.\r\n");

	while (1) {
		cli();
		uint8_t m = current_mode;
		sei();
		if (m != 1) {
			state = S_SHOW_MENU;
			return;
		}

		switch (state) {
			case S_SHOW_MENU:
			writeString("\r\n--- Selecciona servo ---\r\n");
			writeString("  1) Servo D3\r\n");
			writeString("  2) Servo D9\r\n");
			writeString("  3) Servo D10\r\n");
			writeString("  4) Servo D11\r\n");
			writeString("Opcion: ");
			state = S_WAIT_SERVO;
			break;

			case S_WAIT_SERVO:
			if (!uart_available()) break;
			opcion = readChar();
			writeChar(opcion);
			writeString("\r\n");
			handle = -1;
			switch (opcion) {
				case '1': handle = h_d3;  nombre = "D3";  break;
				case '2': handle = h_d9;  nombre = "D9";  break;
				case '3': handle = h_d10; nombre = "D10"; break;
				case '4': handle = h_d11; nombre = "D11"; break;
				default:
				writeString("Opcion invalida.\r\n");
				state = S_SHOW_MENU;
				break;
			}
			if (handle >= 0) state = S_ASK_ANGLE;
			break;

			case S_ASK_ANGLE:
			writeString("Servo ");
			writeString(nombre);
			writeString(" | Ingresa angulo (0-180) y Enter: ");
			state = S_WAIT_ANGLE;
			break;

			case S_WAIT_ANGLE: {
				uint8_t angulo = 0;
				int8_t  res    = readUInt8_nb(&angulo, NULL);
				if (res <= 0) break;
				if (angulo > SERVO_ANGLE_MAX) angulo = SERVO_ANGLE_MAX;
				servo_move(handle, angulo);
				writeString(">> Servo ");
				writeString(nombre);
				writeString(" -> ");
				writeInt(angulo);
				writeString(" grados.\r\n");
				handle = -1;
				state  = S_SHOW_MENU;
				break;
			}
		}
	}
}

void mode2_saved_positions(void) {
}

void mode3_default_positions(void) {
}

uint16_t read_adc(uint8_t channel) {
    ADMUX = (1<<REFS0) | (channel & 0x07);
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
    return ADC;
}

/****************************************/
// ISR
/****************************************/
ISR(PCINT2_vect) {
    static uint8_t up_prev   = 1;
    static uint8_t down_prev = 1;

    uint8_t up_curr   = (PIND & (1<<PIN_BTN_UP))  ? 1 : 0;
    uint8_t down_curr = (PIND & (1<<PIN_BTN_DOWN)) ? 1 : 0;

    if (up_curr == 0 && up_prev == 1) {
        current_mode = (current_mode >= MODE_MAX) ? MODE_MIN : current_mode + 1;
        update_mode_leds(current_mode);
    }
    if (down_curr == 0 && down_prev == 1) {
        current_mode = (current_mode <= MODE_MIN) ? MODE_MAX : current_mode - 1;
        update_mode_leds(current_mode);
    }

    up_prev   = up_curr;
    down_prev = down_curr;
}