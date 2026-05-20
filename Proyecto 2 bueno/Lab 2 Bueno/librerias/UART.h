/*
 * UART.h
 *
 * Created: 15/05/2026 15:22:34
 *  Author: luisz
 */
#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <avr/io.h>

void    initUART(void);
void    writeChar(char caracter);
void    writeString(char *string);
void    writeInt(uint16_t num);
char    readChar(void);
uint8_t uart_available(void);
int8_t  readUInt8_nb(uint8_t *out);

#endif /* UART_H_ */