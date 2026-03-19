/*
* Proyecto 1.asm
*
* Creado: 27/02/2028
* Autor : Luis Arriaza
* Descripción: Reloj/Fecha/Alarma
*/
/****************************************/
// Encabezado (Definición de Registros, Variables y Constantes)
.include "M328PDEF.inc"
.equ    T1VALUE  = 0xF423       // Constante para 1 segundo OCR Timer1
.equ    MAX_MODES = 5

.def    MODE     = R20
.def    DIA_DER  = R4
.def    DIA_IZ   = R5
.def    ALR_MIN_DER  = R6
.def    ALR_MIN_IZ   = R7
.def    ALR_HORA_DER = R10
.def    ALR_HORA_IZ  = R11
.def    BUZZER_ON    = R12 
.def    LED_MODE     = R13  
.def    MES_DER  = R18
.def    MES_IZ   = R19
.def    MIN_DER  = R21
.def    MIN_IZ   = R22
.def    HORA_DER = R23
.def    HORA_IZ  = R24
.def    SEG      = R25
.def    BLINK    = R15   //Registro para hacer parpadear displays

.dseg
.org    SRAM_START
//variable_name:     .byte   1   // Memory alocation for variable_name:     .byte   (byte size)
DIAS_MES:   .byte 12 //Tabla hasta el final

.cseg
.org 0x0000
    JMP SETUP
.org PCI1addr
    JMP PCINT_ISR
.org OC1Aaddr
    JMP TMR1_ISR
.org OC0Aaddr
    JMP TMR0_ISR

/****************************************/
// Configuración de la pila
SETUP:
    LDI R16, LOW(RAMEND)
    OUT SPL, R16
    LDI R16, HIGH(RAMEND)
    OUT SPH, R16

/****************************************/
// Configuracion MCU
    CLI

    // Seteo de puertos
    LDI R16, 0xFF
    OUT DDRC, R16               // Puerto C salida
    LDI R16, 0x00
    OUT DDRB, R16               // Puerto B entrada
    LDI R16, 0xFF
    OUT DDRD, R16               // Puerto D salida
    LDI R16, 0xFF
    OUT PORTB, R16              // Pull-ups en B
    LDI R16, 0x00
    OUT PORTC, R16
    OUT PORTD, R16

    // Timer1 – 1 s, CTC, prescaler 256
    LDI R16, 0
    STS TCCR1A, R16
    LDI R16, (1 << WGM12)|(1 << CS12)
    STS TCCR1B, R16
    LDI R16, HIGH(T1VALUE)
    STS OCR1AH, R16
    LDI R16, LOW(T1VALUE)
    STS OCR1AL, R16

    // Timer0 – 10 ms, CTC, prescaler 1024
    LDI R16, (1 << WGM01)
    OUT TCCR0A, R16
    LDI R16, (1 << CS02)|(1 << CS00)
    OUT TCCR0B, R16
    LDI R16, 156
    OUT OCR0A, R16

    // Interrupciones
    LDI R16, (1 << OCIE0A)
    STS TIMSK0, R16
    LDI R16, (1 << OCIE1A)
    STS TIMSK1, R16
    LDI R16, (1 << PCIE0)
    STS PCICR, R16
    LDI R16, (1 << PCINT0)|(1 << PCINT1)|(1 << PCINT2)|(1 << PCINT3)|(1 << PCINT4)
    STS PCMSK0, R16

    // Inicializar registros de tiempo y fecha
    CLR MIN_DER
    CLR MIN_IZ
    CLR HORA_DER
    CLR HORA_IZ
    LDI R16, 1
    MOV DIA_DER, R16
    CLR DIA_IZ
    LDI MES_DER, 1
    CLR MES_IZ
    CLR MODE
    CLR R26
    LDI R16, 1
    MOV BLINK, R16              // BLINK=1 (displays encendidos por defecto)
    CLR R28                     // Sin banderas de botón pendientes
    CLR ALR_MIN_DER
    CLR ALR_MIN_IZ
    CLR ALR_HORA_DER
    CLR ALR_HORA_IZ
    CLR BUZZER_ON
    CLR LED_MODE

    // Copiar tabla de días por mes desde flash a SRAM
    LDI ZH, HIGH(Tabla_dias_mes << 1)
    LDI ZL, LOW(Tabla_dias_mes << 1)
    LDI XH, HIGH(DIAS_MES)
    LDI XL, LOW(DIAS_MES)
    LDI R16, 12                 // 12 bytes (enero-diciembre)
Copy_tabla:
    LPM R17, Z+
    ST  X+, R17
    DEC R16
    BRNE Copy_tabla

    SEI

/****************************************/
// Loop Infinito
MAIN_LOOP:
    CALL Logica_tiempo
    CALL Logica_fecha

    CPI MODE, 0
    BREQ Hora
    CPI MODE, 1
    BREQ Fecha
    CPI MODE, 2
    BREQ config_hora
    CPI MODE, 3
    BREQ config_fecha
    CPI MODE, 4
    BREQ config_alarma
    CPI MODE, 5
    BREQ alarma
// MODO 0: Reloj
Hora:
    CALL Display_hora
    RJMP MAIN_LOOP
// MODO 1: Fecha
Fecha:
    CALL Display_fecha
    RJMP MAIN_LOOP

// MODO 2: Configuración de hora
config_hora:
    CALL Leer_banderas_hora
    TST  BLINK
    BREQ Apagar_display
    CALL Display_hora
    RJMP MAIN_LOOP

// MODO 3: Configuración de fecha
config_fecha:
    CALL Leer_banderas_fecha
    TST  BLINK
    BREQ Apagar_display
    CALL Display_fecha
    RJMP MAIN_LOOP

Apagar_display:
    CALL Display_off
    RJMP MAIN_LOOP

// MODO 4: Configuración de alarma
config_alarma:
    CALL Leer_banderas_alarma
    CALL Display_alarma
    RJMP MAIN_LOOP

// MODO 5: Alarma activa
// Muestra hora. Si hora==alarma activa buzzer PINC5.
alarma:
    CALL Logica_alarma
    CALL Display_hora
    RJMP MAIN_LOOP

/****************************************/
// NON-Interrupt subroutines
//Apagar segmentos y selectores, preservar PD7 según MODE
Display_off: 
    PUSH R16
    IN   R16, PORTC
    ANDI R16, 0xF0              // apagar selectores de dígito (bits 0-3)
    OUT  PORTC, R16
    LDI  R16, 0x00              // segmentos apagados, PD7=0 por defecto
    CPI  MODE, 0
    BREQ doff_pd7_on
    CPI  MODE, 2
    BREQ doff_pd7_on
    RJMP doff_out
doff_pd7_on:
    LDI  R16, 0x80              // PD7=1 para modos hora
doff_out:
    OUT  PORTD, R16
    POP  R16
    RET
// Lógica de tiempo
// Min max 59, hora max 23
Logica_tiempo:
    CPI SEG, 60
    BREQ Minuto
EXIT_logica:
    RET
Minuto:
    CLR		SEG
    CPI     MIN_DER, 9
    BREQ    overflow1
    INC     MIN_DER
    ANDI    MIN_DER, 0x0F
    RJMP    EXIT_logica
overflow1:
    CLR MIN_DER
    CPI MIN_IZ, 5
    BREQ overflow2
    INC MIN_IZ
    ANDI MIN_IZ, 0x0F
    RJMP EXIT_logica
overflow2:
    CLR MIN_IZ
    CPI HORA_IZ, 2
    BREQ overflow3
    CPI HORA_DER, 9
    BREQ overflow4
    INC HORA_DER
    ANDI HORA_DER, 0x0F
    RJMP EXIT_logica
overflow3:
    CPI HORA_DER, 3
    BREQ overflow5
    INC HORA_DER
    ANDI HORA_DER, 0x0F
    RJMP EXIT_logica
overflow4:
    CLR HORA_DER
    INC HORA_IZ
    ANDI HORA_IZ, 0x0F
    RJMP EXIT_logica
overflow5:
    CLR HORA_DER
    CLR HORA_IZ
    RJMP EXIT_logica

// Lógica de fecha:
// Incrementa día al dar medianoche hace overflow según días del mes máx
// Registros usados internamente: R16, XH/XL, ZH/ZL
Logica_fecha:
    // Solo actuar en media noche
    CPI SEG, 0
    BRNE lf_salir
    CPI MIN_IZ, 0
    BRNE lf_salir
    CPI MIN_DER, 0
    BRNE lf_salir
    CPI HORA_IZ, 0
    BRNE lf_salir
    CPI HORA_DER, 0
    BRNE lf_salir
    RJMP lf_procesar        // está en medianoche, procesar fecha
lf_salir:
    RJMP EXIT_log_fecha

lf_procesar:
// Calcular número de mes como entero: MES_IZ*10 + MES_DER
    PUSH R16
    PUSH R17
    PUSH XH
    PUSH XL

    MOV  R17, MES_IZ
    LDI  R16, 10
    MUL  R17, R16              
    MOV  R17, R0                
    ADD  R17, MES_DER           //R17 = mes_val

    // Leer día máximo del mes: DIAS_MES[mes_val - 1]
    LDI  XH, HIGH(DIAS_MES)
    LDI  XL, LOW(DIAS_MES)
    DEC  R17                   
    ADD  XL, R17                // X apunta a DIAS_MES + (mes-1)
    ADC  XH, R1
    LD   R16, X                 // R16 = días máximos del mes actual
    INC  R17                    // restaurar R17 para cálculo de día

 // Calcular día actual como entero: DIA_IZ*10 + DIA_DER
    MOV  R17, DIA_IZ
    LDI  XL, 10               
    MUL  R17, XL
    MOV  R17, R0
    ADD  R17, DIA_DER          

    // Comparar día actual con máximo del mes
    CP   R17, R16
    BRLO log_fecha_inc_dia      // día < máximo, solo incrementar día
    BREQ log_fecha_overflow_dia // día == máximo, overflow de día

log_fecha_overflow_dia:
    // Resetear día a 01 e incrementar mes
    LDI  R16, 1
    MOV  DIA_DER, R16
    CLR  DIA_IZ

    // Incrementar mes
    // Recalcular mes_val desde los registros actuales
    MOV  R17, MES_IZ
    LDI  XL, 10
    MUL  R17, XL
    MOV  R17, R0
    ADD  R17, MES_DER           // R17 = mes actual como entero
    CPI  R17, 12
    BREQ log_fecha_overflow_mes
    // Verfifiar overflow mes_der
    CPI  MES_DER, 9
    BREQ log_fecha_mes_carry
    INC  MES_DER
    ANDI MES_DER, 0x0F
    RJMP log_fecha_exit
log_fecha_mes_carry:
    CLR  MES_DER
    INC  MES_IZ
    ANDI MES_IZ, 0x0F
    RJMP log_fecha_exit
//Verifica overflow mes_iz
log_fecha_overflow_mes:
    LDI  R16, 1
    MOV  MES_DER, R16
    CLR  MES_IZ
    RJMP log_fecha_exit

//Incrementa dia normal
log_fecha_inc_dia:
    MOV R16, DIA_DER
    CPI R16, 9
	//Revisa carry
    BREQ log_fecha_dia_carry
    INC  DIA_DER
    LDI R16, 0x0F
    AND DIA_DER, R16
    RJMP log_fecha_exit
log_fecha_dia_carry:
    CLR  DIA_DER
    INC  DIA_IZ
    LDI R16, 0x0F
    AND DIA_IZ, R16

log_fecha_exit:
    POP  XL
    POP  XH
    POP  R17
    POP  R16

EXIT_log_fecha:
    RET

// Config_hora: Lee R28, ejecuta la acción correspondiente y limpia R28.
//   0x01, incrementa MIN_DER
//   0x02, decrementa MIN_DER
//   0x03, incrementa unidades de hora HORA_DER
//   0x04, decrementa HORA_DER
Leer_banderas_hora:
    PUSH R16
    MOV  R16, R28
    CLR  R28                        // limpiar bandera

    CPI  R16, 0x01
    BREQ cfg_hora_inc_min
    CPI  R16, 0x02
    BREQ cfg_hora_dec_min
    CPI  R16, 0x03
    BREQ cfg_hora_inc_hora
    CPI  R16, 0x04
    BREQ cfg_hora_dec_hora
    RJMP cfg_hora_exit              // ninguna bandera activa

//Incremento de minuto
cfg_hora_inc_min:
    // Overflow
    CPI MIN_IZ, 5
    BRNE cfg_hora_inc_min_cont
    CPI MIN_DER, 9
    BRNE cfg_hora_inc_min_cont
    CLR MIN_DER                     
    CLR MIN_IZ
    RJMP cfg_hora_exit_min
cfg_hora_inc_min_cont:
    CPI MIN_DER, 9
    BREQ cfg_hora_inc_min_carry
    INC MIN_DER
    ANDI MIN_DER, 0x0F
    RJMP cfg_hora_exit_min
cfg_hora_inc_min_carry:
    CLR MIN_DER
    INC MIN_IZ
    ANDI MIN_IZ, 0x0F
    RJMP cfg_hora_exit_min

//Decremento de minuto
cfg_hora_dec_min:
    // Underflow
    CPI MIN_IZ, 0
    BRNE cfg_hora_dec_min_cont
    CPI MIN_DER, 0
    BREQ cfg_hora_dec_min_wrap
cfg_hora_dec_min_cont:
    CPI MIN_DER, 0
    BREQ cfg_hora_dec_min_borrow
    DEC MIN_DER
    ANDI MIN_DER, 0x0F
    RJMP cfg_hora_exit_min
cfg_hora_dec_min_borrow:
    LDI R16, 9
    MOV MIN_DER, R16
    DEC MIN_IZ
    ANDI MIN_IZ, 0x0F
    RJMP cfg_hora_exit_min
cfg_hora_dec_min_wrap:             
    LDI R16, 9
    MOV MIN_DER, R16
    LDI R16, 5
    MOV MIN_IZ, R16
cfg_hora_exit_min:
    RJMP cfg_hora_exit

//Incremento de hora
cfg_hora_inc_hora:
    // Overflow
    CPI HORA_IZ, 2
    BRNE cfg_hora_inc_hora_cont
    CPI HORA_DER, 3
    BRNE cfg_hora_inc_hora_cont
    CLR HORA_DER                   
    CLR HORA_IZ
    RJMP cfg_hora_exit_hora
cfg_hora_inc_hora_cont:
    CPI HORA_IZ, 2
    BREQ cfg_hora_inc_hora_2x       
    CPI HORA_DER, 9
    BREQ cfg_hora_inc_hora_carry
    INC HORA_DER
    ANDI HORA_DER, 0x0F
    RJMP cfg_hora_exit_hora
cfg_hora_inc_hora_2x:
    INC HORA_DER
    ANDI HORA_DER, 0x0F
    RJMP cfg_hora_exit_hora
cfg_hora_inc_hora_carry:
    CLR HORA_DER
    INC HORA_IZ
    ANDI HORA_IZ, 0x0F
    RJMP cfg_hora_exit_hora

// Decremento de hora
cfg_hora_dec_hora:
    // Underflow
    CPI HORA_IZ, 0
    BRNE cfg_hora_dec_hora_cont
    CPI HORA_DER, 0
    BREQ cfg_hora_dec_hora_wrap
cfg_hora_dec_hora_cont:
    CPI HORA_DER, 0
    BREQ cfg_hora_dec_hora_borrow
    DEC HORA_DER
    ANDI HORA_DER, 0x0F
    RJMP cfg_hora_exit_hora
cfg_hora_dec_hora_borrow:
    LDI R16, 9
    MOV HORA_DER, R16
    DEC HORA_IZ
    ANDI HORA_IZ, 0x0F
    RJMP cfg_hora_exit_hora
cfg_hora_dec_hora_wrap:            
    LDI R16, 3
    MOV HORA_DER, R16
    LDI R16, 2
    MOV HORA_IZ, R16
cfg_hora_exit_hora:
    RJMP cfg_hora_exit

cfg_hora_exit:
    POP R16
    RET


// Leer_banderas_fecha:
//   0x01, mes++ 
//   0x02, mes--  
//   0x03, día++  
//   0x04, día--  
Leer_banderas_fecha:
    PUSH R16
    PUSH R17
    PUSH XH
    PUSH XL
    MOV  R16, R28
    CLR  R28

    CPI  R16, 0x01
    BRNE lbf_no01
    JMP  cfg_fecha_inc_mes
lbf_no01:
    CPI  R16, 0x02
    BRNE lbf_no02
    JMP  cfg_fecha_dec_mes
lbf_no02:
    CPI  R16, 0x03
    BRNE lbf_no03
    JMP  cfg_fecha_inc_dia
lbf_no03:
    CPI  R16, 0x04
    BREQ lbf_es04
    JMP  cfg_fecha_exit
lbf_es04:
    JMP  cfg_fecha_dec_dia

//R17 = días máximos del mes actual
cfg_fecha_get_max_dia:
    MOV  R17, MES_IZ
    LDI  R16, 10
    MUL  R17, R16
    MOV  R17, R0
    ADD  R17, MES_DER           // R17 = mes entero (1-12)
    DEC  R17                    // índice base-0 (0-11)
    LDI  XH, HIGH(DIAS_MES)
    LDI  XL, LOW(DIAS_MES)
    ADD  XL, R17
    ADC  XH, R1
    LD   R17, X                 // R17 = días máx del mes
    RET

//Incremento de mes
cfg_fecha_inc_mes:
    CPI  MES_IZ, 1
    BRNE cfg_fecha_inc_mes_cont
    CPI  MES_DER, 2
    BRNE cfg_fecha_inc_mes_cont
    LDI  R16, 1                 //overflow de mes
    MOV  MES_DER, R16
    CLR  MES_IZ
    JMP  cfg_fecha_reset_dia
cfg_fecha_inc_mes_cont:
    CPI  MES_DER, 9
    BREQ cfg_fecha_inc_mes_carry
    INC  MES_DER
    ANDI MES_DER, 0x0F
    JMP  cfg_fecha_reset_dia
cfg_fecha_inc_mes_carry:
    CLR  MES_DER
    INC  MES_IZ
    ANDI MES_IZ, 0x0F
    JMP  cfg_fecha_reset_dia

// Decremento de mes
cfg_fecha_dec_mes:
    CPI  MES_IZ, 0
    BRNE cfg_fecha_dec_mes_cont
    CPI  MES_DER, 1
    BRNE cfg_fecha_dec_mes_cont
    LDI  R16, 2                 //underflow de mes
    MOV  MES_DER, R16
    LDI  R16, 1
    MOV  MES_IZ, R16
    JMP  cfg_fecha_reset_dia
cfg_fecha_dec_mes_cont:
    CPI  MES_DER, 0
    BREQ cfg_fecha_dec_mes_borrow
    DEC  MES_DER
    ANDI MES_DER, 0x0F
    JMP  cfg_fecha_reset_dia
cfg_fecha_dec_mes_borrow:
    LDI  R16, 9
    MOV  MES_DER, R16
    DEC  MES_IZ
    ANDI MES_IZ, 0x0F
    JMP  cfg_fecha_reset_dia

//Reset día a 01 al cambiar mes
cfg_fecha_reset_dia:
    LDI  R16, 1
    MOV  DIA_DER, R16
    CLR  DIA_IZ
    JMP  cfg_fecha_exit

//Incremento de día
cfg_fecha_inc_dia:
    CALL cfg_fecha_get_max_dia  // R17 = max días del mes actual
    MOV  R16, DIA_IZ
    LDI  XL, 10
    MUL  R16, XL
    MOV  R16, R0
    ADD  R16, DIA_DER           // R16 = día actual entero
    CP   R16, R17
    BRNE cfg_fecha_inc_dia_cont
    LDI  R16, 1                 //overflow dia max
    MOV  DIA_DER, R16
    CLR  DIA_IZ
    JMP  cfg_fecha_exit
cfg_fecha_inc_dia_cont:
    MOV R16, DIA_DER
    CPI R16, 9
    BREQ cfg_fecha_inc_dia_carry
    INC  DIA_DER
    LDI R16, 0x0F
    AND DIA_DER, R16
    JMP  cfg_fecha_exit
cfg_fecha_inc_dia_carry:
    CLR  DIA_DER
    INC  DIA_IZ
    LDI R16, 0x0F
    AND DIA_IZ, R16
    JMP  cfg_fecha_exit

//Decremento de día
cfg_fecha_dec_dia:
    MOV R16, DIA_IZ
    CPI R16, 0
    BRNE cfg_fecha_dec_dia_cont
    MOV R16, DIA_DER
    CPI R16, 1
    BRNE cfg_fecha_dec_dia_cont
    CALL cfg_fecha_get_max_dia  //underflow de dia 01 a máx
    CLR  DIA_IZ
cfg_fecha_dec_dia_div:          // dividir R17 entre 10 para separar a decenas y unidades de regreso
    CPI  R17, 10
    BRLO cfg_fecha_dec_dia_done
    SUBI R17, 10
    INC  DIA_IZ
    JMP  cfg_fecha_dec_dia_div
cfg_fecha_dec_dia_done:
    MOV  DIA_DER, R17
    JMP  cfg_fecha_exit
cfg_fecha_dec_dia_cont:
    MOV R16, DIA_DER
    CPI R16, 0
    BREQ cfg_fecha_dec_dia_borrow
    DEC  DIA_DER
    LDI R16, 0x0F
    AND DIA_DER, R16
    JMP  cfg_fecha_exit
cfg_fecha_dec_dia_borrow:
    LDI  R16, 9
    MOV  DIA_DER, R16
    DEC  DIA_IZ
    LDI R16, 0x0F
    AND DIA_IZ, R16
    JMP  cfg_fecha_exit

cfg_fecha_exit:
    POP  XL
    POP  XH
    POP  R17
    POP  R16
    RET


/****************************************/
//Display de alarma
Display_alarma:
    MOV R30, ALR_MIN_DER
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 2
    SBI PORTC, 3
    CALL Display
    CALL DELAY
    MOV R30, ALR_MIN_IZ
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 3
    SBI PORTC, 2
    CALL Display
    CALL DELAY
    MOV R30, ALR_HORA_DER
    CBI PORTC, 0
    CBI PORTC, 2
    CBI PORTC, 3
    SBI PORTC, 1
    CALL Display
    CALL DELAY2
    MOV R30, ALR_HORA_IZ
    CBI PORTC, 1
    CBI PORTC, 3
    CBI PORTC, 2
    SBI PORTC, 0
    CALL Display
    CALL DELAY2
    RET

// Leer_banderas_alarma
// Misma lógica que Leer_banderas_hora pero opera sobre ALR_*
//   0x01, ALR_MIN++
//   0x02, ALR_MIN--
//   0x03, ALR_HORA++
//   0x04, ALR_HORA--
// ?????????????????????????????????????????????????????????????????????????????
Leer_banderas_alarma:
    PUSH R16
    MOV  R16, R28
    CLR  R28

    CPI  R16, 0x01
    BRNE lba_no01
    JMP  alr_inc_min
lba_no01:
    CPI  R16, 0x02
    BRNE lba_no02
    JMP  alr_dec_min
lba_no02:
    CPI  R16, 0x03
    BRNE lba_no03
    JMP  alr_inc_hora
lba_no03:
    CPI  R16, 0x04
    BREQ lba_es04
    JMP  alr_exit
lba_es04:
    JMP  alr_dec_hora

//Inc minuto alarma
alr_inc_min:
    MOV  R16, ALR_MIN_IZ
    CPI  R16, 5
    BRNE alr_inc_min_cont
    MOV  R16, ALR_MIN_DER
    CPI  R16, 9
    BRNE alr_inc_min_cont
    CLR  ALR_MIN_DER            //Overflow
    CLR  ALR_MIN_IZ
    JMP  alr_exit
alr_inc_min_cont:
    MOV  R16, ALR_MIN_DER
    CPI  R16, 9
    BREQ alr_inc_min_carry
    INC  ALR_MIN_DER
    LDI  R16, 0x0F
    AND  ALR_MIN_DER, R16
    JMP  alr_exit
alr_inc_min_carry:
    CLR  ALR_MIN_DER
    INC  ALR_MIN_IZ
    LDI  R16, 0x0F
    AND  ALR_MIN_IZ, R16
    JMP  alr_exit

// Dec minuto alarma
alr_dec_min:
    MOV  R16, ALR_MIN_IZ
    CPI  R16, 0
    BRNE alr_dec_min_cont
    MOV  R16, ALR_MIN_DER
    CPI  R16, 0
    BRNE alr_dec_min_cont
    LDI  R16, 9                 //Underflow
    MOV  ALR_MIN_DER, R16
    LDI  R16, 5
    MOV  ALR_MIN_IZ, R16
    JMP  alr_exit
alr_dec_min_cont:
    MOV  R16, ALR_MIN_DER
    CPI  R16, 0
    BREQ alr_dec_min_borrow
    DEC  ALR_MIN_DER
    LDI  R16, 0x0F
    AND  ALR_MIN_DER, R16
    JMP  alr_exit
alr_dec_min_borrow:
    LDI  R16, 9
    MOV  ALR_MIN_DER, R16
    DEC  ALR_MIN_IZ
    LDI  R16, 0x0F
    AND  ALR_MIN_IZ, R16
    JMP  alr_exit

//Inc hora alarma
alr_inc_hora:
    MOV  R16, ALR_HORA_IZ
    CPI  R16, 2
    BRNE alr_inc_hora_cont
    MOV  R16, ALR_HORA_DER
    CPI  R16, 3
    BRNE alr_inc_hora_cont
    CLR  ALR_HORA_DER           //overflow hora
    CLR  ALR_HORA_IZ
    JMP  alr_exit
alr_inc_hora_cont:
    MOV  R16, ALR_HORA_IZ
    CPI  R16, 2
    BREQ alr_inc_hora_2x
    MOV  R16, ALR_HORA_DER
    CPI  R16, 9
    BREQ alr_inc_hora_carry
    INC  ALR_HORA_DER
    LDI  R16, 0x0F
    AND  ALR_HORA_DER, R16
    JMP  alr_exit
alr_inc_hora_2x:
    INC  ALR_HORA_DER
    LDI  R16, 0x0F
    AND  ALR_HORA_DER, R16
    JMP  alr_exit
alr_inc_hora_carry:
    CLR  ALR_HORA_DER
    INC  ALR_HORA_IZ
    LDI  R16, 0x0F
    AND  ALR_HORA_IZ, R16
    JMP  alr_exit

//Dec hora alarma
alr_dec_hora:
    MOV  R16, ALR_HORA_IZ
    CPI  R16, 0
    BRNE alr_dec_hora_cont
    MOV  R16, ALR_HORA_DER
    CPI  R16, 0
    BRNE alr_dec_hora_cont
    LDI  R16, 3                //underflow hora
    MOV  ALR_HORA_DER, R16
    LDI  R16, 2
    MOV  ALR_HORA_IZ, R16
    JMP  alr_exit
alr_dec_hora_cont:
    MOV  R16, ALR_HORA_DER
    CPI  R16, 0
    BREQ alr_dec_hora_borrow
    DEC  ALR_HORA_DER
    LDI  R16, 0x0F
    AND  ALR_HORA_DER, R16
    JMP  alr_exit
alr_dec_hora_borrow:
    LDI  R16, 9
    MOV  ALR_HORA_DER, R16
    DEC  ALR_HORA_IZ
    LDI  R16, 0x0F
    AND  ALR_HORA_IZ, R16

alr_exit:
    POP  R16
    RET

// Logica_alarma:
// Si hay bandera en R28, apagar buzzer y limpiar bandera
// Comparar hora actual con alarma, si coinciden activar buzzer
// Si BUZZER_ON=1 mantener buzzer encendido aunque la hora ya no coincida
Logica_alarma:
    PUSH R16

    //verificar si hay botón presionado
    TST  R28
    BREQ logica_alr_comparar    // sin bandera, continuar a comparación
    CLR  R28                    // limpiar bandera
    CLR  BUZZER_ON
    CBI  PORTC, 5               // apagar buzzer
    POP  R16
    RET

// Si el buzzer ya está activo, mantenerlo encendido
logica_alr_comparar:
    TST  BUZZER_ON
    BREQ logica_alr_check       // no estaba activo, verificar coincidencia
    SBI  PORTC, 5               // mantener buzzer encendido
    POP  R16
    RET

// Comparar hora actual con alarma
logica_alr_check:
    CP   MIN_DER, ALR_MIN_DER
    BRNE logica_alr_exit
    CP   MIN_IZ, ALR_MIN_IZ
    BRNE logica_alr_exit
    CP   HORA_DER, ALR_HORA_DER
    BRNE logica_alr_exit
    CP   HORA_IZ, ALR_HORA_IZ
    BRNE logica_alr_exit     // Coincide, activar buzzer
    LDI  R16, 1
    MOV  BUZZER_ON, R16
    SBI  PORTC, 5

logica_alr_exit:
    POP  R16
    RET

// Display de hora
Display_hora:
    MOV R30, MIN_DER
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 2
    SBI PORTC, 3
    CALL Display
    CALL DELAY
    MOV R30, MIN_IZ
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 3
    SBI PORTC, 2
    CALL Display
    CALL DELAY
    MOV R30, HORA_DER
    CBI PORTC, 0
    CBI PORTC, 2
    CBI PORTC, 3
    SBI PORTC, 1
    CALL Display
    CALL DELAY2
    MOV R30, HORA_IZ
    CBI PORTC, 1
    CBI PORTC, 3
    CBI PORTC, 2
    SBI PORTC, 0
    CALL Display
    CALL DELAY2
    RET

// Display de fecha
Display_fecha:
    MOV R30, MES_DER
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 2
    SBI PORTC, 3
    CALL Display
    CALL DELAY
    MOV R30, MES_IZ
    CBI PORTC, 0
    CBI PORTC, 1
    CBI PORTC, 3
    SBI PORTC, 2
    CALL Display
    CALL DELAY
    MOV R30, DIA_DER
    CBI PORTC, 0
    CBI PORTC, 2
    CBI PORTC, 3
    SBI PORTC, 1
    CALL Display
    CALL DELAY2
    MOV R30, DIA_IZ
    CBI PORTC, 1
    CBI PORTC, 3
    CBI PORTC, 2
    SBI PORTC, 0
    CALL Display
    CALL DELAY2
    RET

DELAY:
    LDI R29, 8
Delay_outer:
    LDI R27, 250
Delay_inner:
    DEC R27
    BRNE Delay_inner
    DEC R29
    BRNE Delay_outer
    RET

DELAY2:
    LDI R29, 15
Delay_outer2:
    LDI R27, 250
Delay_inner2:
    DEC R27
    BRNE Delay_inner2
    DEC R29
    BRNE Delay_outer2
    RET

DISPLAY:
    CPI R30, 0x00
    BRBS 1, Cero
    CPI R30, 0x01
    BRBS 1, Uno
    CPI R30, 0x02
    BRBS 1, Dos
    CPI R30, 0x03
    BRBS 1, Tres
    CPI R30, 0x04
    BRBS 1, Cuatro
    CPI R30, 0x05
    BRBS 1, Cinco
    CPI R30, 0x06
    BRBS 1, Seis
    CPI R30, 0x07
    BRBS 1, Siete
    CPI R30, 0x08
    BRBS 1, Ocho
    CPI R30, 0x09
    BRBS 1, Nueve
    CPI R30, 0x0A
    BRBS 1, Diez
    CPI R30, 0x0B
    BRBS 1, Once
    CPI R30, 0x0C
    BRBS 1, Doce
    CPI R30, 0x0D
    BRBS 1, Trece
    CPI R30, 0x0E
    BRBS 1, Catorce
    CPI R30, 0x0F
    BRBS 1, Quince
    RET

Cero:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b00111111
    RJMP Mostrar_disp
Uno:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b00000110
    RJMP Mostrar_disp
Dos:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01011011
    RJMP Mostrar_disp
Tres:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01001111
    RJMP Mostrar_disp
Cuatro:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01100110
    RJMP Mostrar_disp
Cinco:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01101101
    RJMP Mostrar_disp
Seis:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01111101
    RJMP Mostrar_disp
Siete:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b00000111
    RJMP Mostrar_disp
Ocho:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01111111
    RJMP Mostrar_disp
Nueve:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01101111
    RJMP Mostrar_disp
Diez:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01110111
    RJMP Mostrar_disp
Once:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01111100
    RJMP Mostrar_disp
Doce:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b00111001
    RJMP Mostrar_disp
Trece:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01011110
    RJMP Mostrar_disp
Catorce:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01111001
    RJMP Mostrar_disp
Quince:
    IN R31, PORTD
    ANDI R31, 0x80
    ORI  R31, 0b01110001
    RJMP Mostrar_disp

Mostrar_disp:
    ANDI R31, 0x7F              // limpiar PD7
    CPI  MODE, 0
    BREQ mostrar_pd7_on
    CPI  MODE, 2
    BREQ mostrar_pd7_on			// Modos 4 y 5: PD7 según LED_MODE
    CPI  MODE, 4
    BREQ mostrar_pd7_led
    CPI  MODE, 5
    BREQ mostrar_pd7_led
    RJMP mostrar_pd7_out        // modos 1 y3, PD7 = 0
mostrar_pd7_led:
    TST  LED_MODE               //Si es 0 apagado, no es 0 encendido
    BREQ mostrar_pd7_out
mostrar_pd7_on:
    ORI  R31, 0x80              // PD7 = 1
mostrar_pd7_out:
    OUT  PORTD, R31
    RET

/****************************************/
// Interrupt routines
// Interrupcion botón:
// Detecta qué botón se presionó con antirrebote. Cambio de modo cíclico si se presiona boton de modo.
// Al entrar a modo 2 desahabilita Timer1, al salir de modo 3 rehabilita Timer1
// Carga la bandera en R28 para ser procesada por el main loop de los demas botones
PCINT_ISR:
    PUSH R16
    IN   R16, SREG
    PUSH R16
    PUSH R17
    PUSH R18
    PUSH R19
    PUSH R23
    PUSH R24

    IN R16, PINB

Delay3:                         // antirrebote
    LDI R17, 25
Delay_outer3:
    LDI R18, 255
Delay_inner3:
    DEC R18
    BRNE Delay_inner3
    DEC R17
    BRNE Delay_outer3
    IN R19, PINB
    CP R19, R16
    BRNE EXIT_PCINT

    ANDI R16, 0x1F
    CPI  R16, 0b00011011
    BREQ Cambio_modo
    CPI  R16, 0b00011101
    BREQ Inc_disp_der
    CPI  R16, 0b00011110
    BREQ Dec_disp_der
    CPI  R16, 0b00001111
    BREQ Inc_disp_iz
    CPI  R16, 0b00010111
    BREQ Dec_disp_iz
    RJMP EXIT_PCINT

EXIT_PCINT:
    POP R24
    POP R23
    POP R19
    POP R18
    POP R17
    POP R16
    OUT SREG, R16
    POP R16
    RETI

// Cambio de modo
Cambio_modo:
    // Salir de modo 5, apagar buzzer
    CPI MODE, 5
    BRNE cambio_modo_no5
    CLR  BUZZER_ON
    CBI  PORTC, 5
cambio_modo_no5:
    //Salir de modo 3, rehabilitar Timer1
    CPI MODE, 3
    BREQ Cambio_modo_reh_tmr1
    RJMP Cambio_modo_inc

Cambio_modo_reh_tmr1:           // Rehabilitar Timer1 al salir de modo config
    PUSH R16
    LDI  R16, (1 << OCIE1A)
    STS  TIMSK1, R16
    POP  R16

Cambio_modo_inc:
    CPI MODE, 5
    BREQ mode_overflow
    INC  MODE

    // Nuevo modo modo es 2, deshabilitar Timer1
    CPI MODE, 2
    BREQ Cambio_modo_dis_tmr1
	//Nuevo modo modo es 4 o 5, led alarma
    CPI MODE, 4
    BREQ Cambio_modo_alarma
    CPI MODE, 5
    BREQ Cambio_modo_alarma
    RJMP EXIT_PCINT

// Entrar a modo 4 o 5: LED encendido, contadores reseteados
Cambio_modo_alarma:             
    LDI  R16, 1
    MOV  LED_MODE, R16          // LED encendido al entrar
    CLR  R26                    // resetear contador 500ms
    RJMP EXIT_PCINT

Cambio_modo_dis_tmr1:
    PUSH R16
    LDI  R16, 0
    STS  TIMSK1, R16                // deshabilitar Timer1
    LDI  R16, 1
    MOV  BLINK, R16                 // displays encendidos al entrar
    CLR  R26                        // resetear contador blink para que el primer toggle sea a 500ms
    CLR  R8                         // resetear contador blink 250ms
    POP  R16
    RJMP EXIT_PCINT

// Si modo era 5, overflow a 0
mode_overflow:
    CLR  MODE
    RJMP EXIT_PCINT


//Bandera de botones
Inc_disp_der:
    LDI R28, 0x01
    RJMP EXIT_PCINT
Dec_disp_der:
    LDI R28, 0x02
    RJMP EXIT_PCINT
Inc_disp_iz:
    LDI R28, 0x03
    RJMP EXIT_PCINT
Dec_disp_iz:
    LDI R28, 0x04
    RJMP EXIT_PCINT

//Timer1 ISR: incrementa segundos y toggle LED modo 4 cada 1s
TMR1_ISR:
    PUSH R16
    IN   R16, SREG
    PUSH R16
    INC  SEG
    CPI  MODE, 4
    BRNE Exit_tm1
    // Toggle LED_MODE
    TST  LED_MODE
    BREQ tm1_led_on
    CLR  LED_MODE
    RJMP Exit_tm1
tm1_led_on:
    LDI  R16, 1
    MOV  LED_MODE, R16
Exit_tm1:
    POP R16
    OUT SREG, R16
    POP R16
    RETI

//Timer0 ISR: blink 250ms en modos 2 y 3. Toggle PINC,4 cada 500ms siempre
TMR0_ISR:
    PUSH R16
    IN   R16, SREG
    PUSH R16

// Contador 500ms para PINC4 y LED modo alarma
    INC  R26
    CPI  R26, 50
    BRNE tm0_check_blink
    CLR  R26
    SBI  PINC, 4                   // toggle PINC,4 cada 500ms
    CPI  MODE, 5
    BRNE tm0_check_blink			// Toggle LED_MODE para modo 5
    TST  LED_MODE
    BREQ tm0_led_on
    CLR  LED_MODE
    RJMP tm0_check_blink
tm0_led_on:
    LDI  R16, 1
    MOV  LED_MODE, R16

// Contador 250ms para BLINK en modo 2 y 3
tm0_check_blink:

    INC  R8
    MOV  R16, R8
    CPI  R16, 25
    BRNE EXIT_tm0
    CLR  R8

    CPI  MODE, 2
    BREQ blink_toggle
    CPI  MODE, 3
    BREQ blink_toggle
    RJMP EXIT_tm0

blink_toggle:
    TST  BLINK
    BREQ blink_set_uno
    CLR  BLINK
    RJMP EXIT_tm0
blink_set_uno:
    LDI  R16, 1
    MOV  BLINK, R16

EXIT_tm0:
    POP R16
    OUT SREG, R16
    POP R16
    RETI
/****************************************/
// Tabla de días máximos por mes: indice 0 = enero, 1 = febrero,... 12 = diciembre
//Se guardo hasta acá por la ubicacion en la SRAM
Tabla_dias_mes:
    .db 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
