/*
 * Lab 5 digi 2 nano.c
 *
 * Created: 03/09/2026 15:30:32
 * Author : luisz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "UART.h"

// Variables de estado del control (0: suelto, 1: presionado)
volatile uint8_t up = 0;
volatile uint8_t down = 0;
volatile uint8_t left = 0;
volatile uint8_t right = 0;
volatile uint8_t bot_A = 0;
volatile uint8_t bot_B = 0;

// Mantiene el estado previo de PIND para detectar cuáles pines cambiaron
static uint8_t estado_previo_pind = 0xFF;

void initButtonsPCINT(void);
void evaluarBoton(uint8_t pin_mask, volatile uint8_t *variable, uint8_t estado_actual, const char *nombre_btn);

int main(void)
{
    initUART();
    initButtonsPCINT();
    
    // Habilitar interrupciones globales
    sei();

    while (1) 
    {
        // Loop principal libre para otras tareas
    }
}

void initButtonsPCINT(void)
{
    // Configurar D2-D7 (PD2-PD7) como entradas
    DDRD &= ~((1 << DDD2) | (1 << DDD3) | (1 << DDD4) | (1 << DDD5) | (1 << DDD6) | (1 << DDD7));
    
    // Activar resistencias pull-up internas en D2-D7
    PORTD |= (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7);
    
    // Habilitar la interrupción PCIE2 (cubre el grupo PCINT[23:16] / PORTD)
    PCICR |= (1 << PCIE2);
    
    // Habilitar máscaras de interrupción específicas para D2-D7 (PCINT18 a PCINT23)
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22) | (1 << PCINT23);
    
    // Guardar estado inicial del puerto D
    estado_previo_pind = PIND;
}

// ISR para interrupción por cambio de pin en PORTD
ISR(PCINT2_vect)
{
    uint8_t estado_actual = PIND;
    
    // Asignación de pines:
    // D2 (PD2) -> UP
    // D3 (PD3) -> DOWN
    // D4 (PD4) -> LEFT
    // D5 (PD5) -> RIGHT
    // D6 (PD6) -> BOTON A
    // D7 (PD7) -> BOTON B
    
    evaluarBoton((1 << PIND2), &up, estado_actual, "up");
    evaluarBoton((1 << PIND3), &down, estado_actual, "down");
    evaluarBoton((1 << PIND4), &left, estado_actual, "left");
    evaluarBoton((1 << PIND5), &right, estado_actual, "right");
    evaluarBoton((1 << PIND6), &bot_A, estado_actual, "A");
    evaluarBoton((1 << PIND7), &bot_B, estado_actual, "B");

    // Actualizar registro de estado previo
    estado_previo_pind = estado_actual;
}

// Revisa si un pin específico cambió y envía la trama UART si aplica
void evaluarBoton(uint8_t pin_mask, volatile uint8_t *variable, uint8_t estado_actual, const char *nombre_btn)
{
    // Verificar si el pin cambió de estado
    if ((estado_actual & pin_mask) != (estado_previo_pind & pin_mask))
    {
        *variable = !(estado_actual & pin_mask);
        
        writeString((char*)nombre_btn);
        writeChar(*variable ? '1' : '0');
		writeChar('\n');
    }
}