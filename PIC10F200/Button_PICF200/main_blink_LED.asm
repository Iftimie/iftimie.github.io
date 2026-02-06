#include "p10f200.inc"
    __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF

state EQU 0x10     ; 0 = OFF, 1 = BLINK
i     EQU 0x11     ; delay
j EQU 0x12   ; add this near your EQUs

    ORG 0x0000

INIT
    ; enable pull-ups
    MOVLW ~((1<<T0CS)|(1<<NOT_GPPU))
    OPTION

    ; GP1 = output (LED), GP3 = input (button)
    MOVLW ~((1<<GP3)|(1<<GP1))
    TRIS GPIO

    CLRF state
    BCF GPIO, GP1

MAIN
    ; ---- button check ----
    BTFSC GPIO, GP3      ; not pressed?
    GOTO RUN_MODE

    CALL DEBOUNCE
    BTFSC GPIO, GP3
    GOTO RUN_MODE

WAIT_RELEASE
    BTFSS GPIO, GP3
    GOTO WAIT_RELEASE
    CALL DEBOUNCE

    INCF state, F        ; toggle state
    BTFSC state, 0
    GOTO RUN_MODE
    BCF GPIO, GP1        ; ensure OFF

RUN_MODE
    BTFSS state, 0       ; state == 0 ?
    GOTO MAIN            ; OFF mode

    ; ---- BLINK MODE ----
    BSF GPIO, GP1
    CALL BLINK_DELAY
    BCF GPIO, GP1
    CALL BLINK_DELAY
    GOTO MAIN

; ------------------------

DEBOUNCE
    MOVLW D'40'
    MOVWF i
D1
    DECFSZ i, F
    GOTO D1
    RETLW 0

BLINK_DELAY
    MOVLW D'250'
    MOVWF j
OUTER
    MOVLW D'250'
    MOVWF i
INNER
    DECFSZ i, F
    GOTO INNER
    DECFSZ j, F
    GOTO OUTER
    RETLW 0

    END
