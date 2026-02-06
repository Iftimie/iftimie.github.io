#include "p10f200.inc"
    __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF

i   EQU 0x10        ; delay register

    ORG 0x0000

INIT
    ; enable pull-ups, GP3 as input
    MOVLW ~((1<<T0CS)|(1<<NOT_GPPU))
    OPTION

    ; GP1 = output (LED), GP3 = input (button)
    MOVLW ~((1<<GP3)|(1<<GP1))
    TRIS GPIO

    BCF GPIO, GP1   ; LED OFF

MAIN
    BTFSC GPIO, GP3     ; if btn is 0 (pressed)
	    GOTO MAIN           

    CALL DEBOUNCE       ; debounce
    BTFSC GPIO, GP3     ; if still pressed?
    	GOTO MAIN           ; no ? ignore

WAIT_RELEASE
    BTFSS GPIO, GP3     ; if btn is 1 (released)
    	GOTO WAIT_RELEASE

    CALL DEBOUNCE       ; debounce release

    MOVLW (1<<GP1)      ; toggle LED
    XORWF GPIO, F

    GOTO MAIN

; -------------------------
DEBOUNCE
    MOVLW D'40'
    MOVWF i
D_LOOP
    DECFSZ i, F
    GOTO D_LOOP
    RETLW 0

    END
