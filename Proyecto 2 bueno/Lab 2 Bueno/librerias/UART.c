/*
 * UART.c
 *
 * Created: 15/05/2026 15:22:27
 *  Author: luisz
 */
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART.h"

void initUART(void) {
    DDRD &= ~(1<<DDD0);    /* RX entrada */
    DDRD |=  (1<<DDD1);    /* TX salida  */

    UCSR0A = 0;
    /* Sin RXCIE0 — se usa polling, no interrupción de RX */
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    /* Async, sin paridad, 1 stop bit, 8 data bits */
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
    /* 9600 baud @ 16 MHz: UBRR = 16000000/(16*9600) - 1 = 103 */
    UBRR0 = 103;
}

void writeChar(char c) {
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void writeString(char *s) {
    while (*s != '\0') {
        writeChar(*s);
        s++;
    }
}

void writeInt(uint16_t num) {
    char    buf[6];
    uint8_t i = 0;
    if (num == 0) { writeChar('0'); return; }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (uint8_t j = 0; j < i; j++) {
        writeChar(buf[i - 1 - j]);
    }
}

/* 1 si hay byte listo en el buffer UART, 0 si no */
uint8_t uart_available(void) {
    return (UCSR0A & (1<<RXC0)) ? 1 : 0;
}

/* Lectura bloqueante — usar solo tras confirmar uart_available()==1 */
char readChar(void) {
    while (!(UCSR0A & (1<<RXC0)));
    return (char)UDR0;
}

/*
 * readUInt8_nb — lectura no bloqueante de entero 0-255.
 * Llamar repetidamente desde el loop del modo.
 *   Retorna  1: Enter recibido, valor listo en *out.
 *   Retorna  0: esperando más dígitos.
 * Soporta backspace (\b / DEL).
 * Si el valor supera 255 se clampea a 255.
 */
int8_t readUInt8_nb(uint8_t *out) {
    static uint16_t val    = 0;
    static uint8_t  active = 0;

    if (!active) {
        val    = 0;
        active = 1;
    }

    if (!uart_available()) return 0;

    char c = readChar();

    if (c == '\r' || c == '\n') {
        writeString("\r\n");
        if (val > 255) val = 255;
        *out   = (uint8_t)val;
        active = 0;
        val    = 0;
        return 1;
    }
    if (c == '\b' || c == 127) {
        if (val > 0) {
            val /= 10;
            writeString("\b \b");
        }
        return 0;
    }
    if (c >= '0' && c <= '9') {
        uint16_t next = val * 10 + (uint16_t)(c - '0');
        if (next > 255) next = 255;
        val = next;
        writeChar(c);
    }
    return 0;
}