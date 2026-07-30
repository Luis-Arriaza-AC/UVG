/*
 * main.c - MCU1 (MAESTRO)
 * Laboratorio 3: SPI - Parte 1
 *
 * Recibe las lecturas de dos potenciometros desde el esclavo (MCU2)
 * mediante SPI, y las envia a la PC mediante UART para visualizarlas
 * en una terminal serial.
 *
 * ---------------------------------------------------------
 * ASUNCIONES DE PINOUT (corregir si no coinciden con tu circuito):
 *   SPI:  SS = PB2 (D10) | MOSI = PB3 (D11)
 *         MISO = PB4 (D12) | SCK = PB5 (D13)
 *   UART: TX = PD1 (D1) | RX = PD0 (D0)
 *         Baudrate = 9600 (definido por UBRR0 = 103 a 16MHz)
 * ---------------------------------------------------------
 */

#include <avr/io.h>
#include <stdio.h>
#include "libs/SPI.h"
#include "libs/adc.h"
#include "libs/UART.h"

int main(void)
{
    /* -------- Configuracion SPI como MAESTRO -------- */
    spi_config_t spiCfg = {
        .role       = SPI_ROLE_MASTER,
        .data_order = SPI_DATA_ORDER_MSB_FIRST,
        .clock_mode = SPI_MODE0,
        .clock_div  = SPI_CLOCK_DIV16   /* 16MHz/16 = 1MHz de reloj SPI */
    };
    SPI_Init(spiCfg);

    /* -------- Configuracion UART hacia la PC -------- */
    initUART();

    uint8_t pot1, pot2;
    char buffer[32];

    while (1)
    {
		
        /* --- Seleccionamos al esclavo (SS en bajo) --- */
        SPI_PORT &= ~(1 << SPI_SS);

        /* 1ra transaccion: el esclavo responde con la lectura de Pot1.
         * El byte que enviamos (0xFF) es "dummy", solo sirve para
         * generar los pulsos de reloj que el esclavo necesita. */
        pot1 = SPI_MasterTransceive(0xFF);

        /* 2da transaccion: el esclavo responde con la lectura de Pot2 */
        pot2 = SPI_MasterTransceive(0xFF);

        /* --- Liberamos al esclavo (SS en alto) --- */
        SPI_PORT |= (1 << SPI_SS);

        /* --- Enviamos los valores recibidos a la terminal serial --- */
        sprintf(buffer, "Pot1: %3u   Pot2: %3u\r\n", pot1, pot2);
        writeString(buffer);
    }
}

ISR(USART_RX_vect)
{
	char rx = UDR0;
	
	if (rx == 'D' || rx == 'd')
	{
		flag_send_uart = 1;
	}
	else if (rx == '+')
	{
		contador_s3++;
	}
	else if (rx == '-')
	{
		contador_s3--;
	}
}