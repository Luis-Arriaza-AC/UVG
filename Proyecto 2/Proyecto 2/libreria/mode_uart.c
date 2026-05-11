/* mode_uart.c */
#include "mode_uart.h"
#include "UART.h"
#include "servo.h"
#include <stdint.h>

/* ?? Utilidades de impresión ?????????????????????????????????????? */
static void print(const char *s)
{
    writeString((char *)s);
}

static void print_uint8(uint8_t v)
{
    if (v >= 100) writeChar('0' + v / 100);
    if (v >=  10) writeChar('0' + (v / 10) % 10);
    writeChar('0' + v % 10);
}

static void print_menu(void)
{
    print("\r\n=== CONTROL DE SERVOS ===\r\n");
    print("Seleccione servo:\r\n");
    print("  0 -> D9  (Servo 0)\r\n");
    print("  1 -> D10 (Servo 1)\r\n");
    print("  2 -> D11 (Servo 2)\r\n");
    print("  3 -> D3  (Servo 3)\r\n");
    print("Opcion: ");
}

static void print_angle_prompt(uint8_t id)
{
    print("\r\nServo ");
    writeChar('0' + id);
    print(" seleccionado.\r\n");
    print("Angulo (0-90): ");
}

static void print_ok(uint8_t id, uint8_t angle)
{
    print("\r\n>> Servo ");
    writeChar('0' + id);
    print(" -> ");
    print_uint8(angle);
    print(" grados aplicado.\r\n");
}

static void print_error(const char *msg)
{
    print("\r\n[ERROR] ");
    print(msg);
    print("\r\n");
}

/* ?? FSM ?????????????????????????????????????????????????????????? */
typedef enum {
    ST_IDLE,
    ST_WAIT_SERVO,
    ST_WAIT_ANGLE
} menu_state_t;

static menu_state_t menu_state  = ST_IDLE;
static uint8_t      sel_servo   = 0;
static uint16_t     angle_accum = 0;
static uint8_t      digit_count = 0;
/*
 * last_cr: flag para absorber el '\n' que viene después de '\r'
 * en terminales que envían "\r\n". Evita que el '\n' se procese
 * como un segundo evento y duplique el menú.
 */
static uint8_t      last_was_cr = 0;

/* Aplica el ángulo acumulado y regresa al menú de servo */
static void apply_angle(void)
{
    if (angle_accum > 90) {
        print_error("Fuera de rango (0-90). Intente de nuevo.");
        angle_accum = 0;
        digit_count = 0;
        print_angle_prompt(sel_servo);
    } else {
        servo_set_angle((servo_id_t)sel_servo, (uint8_t)angle_accum);
        print_ok(sel_servo, (uint8_t)angle_accum);
        angle_accum = 0;
        digit_count = 0;
        menu_state  = ST_WAIT_SERVO;
        print_menu();
    }
}

/* ?? API pública ?????????????????????????????????????????????????? */
void mode_uart_enter(void)
{
    uart_rx_flush();
    angle_accum = 0;
    digit_count = 0;
    last_was_cr = 0;
    menu_state  = ST_WAIT_SERVO;
    print_menu();
}

void mode_uart_exit(void)
{
    menu_state = ST_IDLE;
    print("\r\n[Modo UART desactivado]\r\n");
}

void mode_uart_run(void)
{
    if (menu_state == ST_IDLE) return;

    char c;
    while (uart_rx_get(&c)) {

        /*
         * Filtro anti-duplicado \r\n:
         * Si el carácter anterior fue '\r' y el actual es '\n',
         * absorbemos el '\n' sin procesarlo en ningún estado.
         */
        if (last_was_cr && c == '\n') {
            last_was_cr = 0;
            continue;
        }
        last_was_cr = (c == '\r');

        switch (menu_state) {

        case ST_WAIT_SERVO:
            if (c >= '0' && c <= '3') {
                writeChar(c);
                sel_servo   = (uint8_t)(c - '0');
                angle_accum = 0;
                digit_count = 0;
                menu_state  = ST_WAIT_ANGLE;
                print_angle_prompt(sel_servo);
            } else if (c == '\r') {
                /* Enter vacío: redibujar menú una sola vez */
                print_menu();
            } else {
                writeChar(c);
                print_error("Use 0, 1, 2 o 3.");
                print_menu();
            }
            break;

        case ST_WAIT_ANGLE:
            if (c >= '0' && c <= '9') {
                writeChar(c);                       /* eco */
                angle_accum = angle_accum * 10u + (uint8_t)(c - '0');
                digit_count++;

                /*
                 * Aplicar automáticamente al llegar a 3 dígitos:
                 * con 3 dígitos ya tenemos el número completo (máx 999,
                 * pero validamos ? 90 dentro de apply_angle).
                 */
                if (digit_count == 3) {
                    apply_angle();
                }
            } else if (c == '\r') {
                if (digit_count == 0) {
                    /* Enter sin dígitos: volver al menú de servo */
                    menu_state = ST_WAIT_SERVO;
                    print_menu();
                } else {
                    /* Enter con al menos 1 dígito: aplicar */
                    apply_angle();
                }
            } else if (c == 0x08 || c == 0x7F) {
                /* Backspace */
                if (digit_count > 0) {
                    digit_count--;
                    angle_accum /= 10u;
                    writeChar(0x08);
                    writeChar(' ');
                    writeChar(0x08);
                }
            }
            /* cualquier otro carácter: ignorar */
            break;

        default:
            break;
        }
    }
}