#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <avr/io.h>

void    initUART(void);
void    writeChar(char caracter);
void    writeString(char *string);
void    writeInt(uint16_t num);
char    readChar(void);
uint8_t readUInt8(void);
uint8_t uart_available(void);   /* ? nuevo: 1 si hay byte listo, 0 si no */

#endif /* UART_H_ */