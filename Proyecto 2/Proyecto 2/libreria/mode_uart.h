/* mode_uart.h */
#ifndef MODE_UART_H
#define MODE_UART_H

#include <stdint.h>

/* Implementadas en main.c — acceso al ring buffer UART */
uint8_t uart_rx_get(char *out);
void    uart_rx_flush(void);

/* Ciclo de vida del modo */
void mode_uart_enter(void);
void mode_uart_run(void);
void mode_uart_exit(void);

#endif /* MODE_UART_H */