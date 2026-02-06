#include "p10f200.inc"
    __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF

counter EQU 0x10
duty    EQU 0x11
j       EQU 0x12


    ORG 0x0000

INIT
    ; GP1 = output
    MOVLW ~(1 << GP1)
    TRIS GPIO

    ; duty = 30% of 255 ˜ 76 (0x4C)
    MOVLW .0
    MOVWF duty

    CLRF counter        ; counter = 0

MAIN_LOOP
    ; if (counter < duty) led_on else led_off

    MOVF  duty, W       ; W = duty
    SUBWF counter, W    ; W = counter - duty

    BTFSS STATUS, C     ; C = 0 ? counter < duty
    CALL LED_ON         ; if counter < duty ? ON

    ; else ? OFF
    BCF GPIO, GP1
    CALL LED_DONE
	
	GOTO MAIN_LOOP

LED_ON
    BSF GPIO, GP1
	RETURN 0

LED_DONE
    ; counter++
    INCF counter, F

    ; if counter >= 255: counter = 0
    INCF counter, F    ; increments and auto-wraps


    ; delay ˜ “step time” (sets PWM freq)
    CALL DELAY

    RETURN 0


; ~some small delay (tune N for desired freq)
DELAY
    MOVLW .10
    MOVWF j
D_LOOP
    DECFSZ j, F
    GOTO D_LOOP
    RETLW 0

    END
