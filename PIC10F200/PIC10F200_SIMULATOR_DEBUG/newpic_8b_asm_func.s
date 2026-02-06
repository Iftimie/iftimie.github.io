#include <xc.inc>
    PROCESSOR 10F200

    CONFIG WDTE = OFF
    CONFIG CP = OFF
    CONFIG MCLRE = OFF

counter  EQU 0x10

    GLOBAL  _main
    GLOBAL  start_initialization

; --- reset / entry glue expected by XC8 ---
    PSECT resetVec,class=CODE,delta=2
resetVec:
    GOTO start_initialization

    PSECT code,class=CODE,delta=2
start_initialization:
    GOTO _main

; --- your program ---
_main:
    MOVLW 0x0D          ; TRIS: GP3,GP2,GP0 inputs; GP1 output
    TRIS  GPIO

loop:
    BSF   GPIO, 1       ; LED ON (GP1)
    CALL  delay
    BCF   GPIO, 1       ; LED OFF
    CALL  delay
    GOTO  loop

delay:
    MOVLW 200
    MOVWF counter
d1:
    DECFSZ counter, F
    GOTO  d1
    RETLW 0

    END
