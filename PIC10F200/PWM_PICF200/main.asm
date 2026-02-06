#include "p10f200.inc"
    __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF

counter EQU 0x10
duty    EQU 0x11
j       EQU 0x12
k       EQU 0x13
l       EQU 0x14
direction EQU 0x15


    ORG 0x0000

INIT
    ; GP1 = output
    MOVLW ~(1 << GP1)
    TRIS GPIO

    CLRF counter        ; counter = 0
	CLRF k
	CLRF l
	CLRF duty
	CLRF direction

DUTY_CHANGE_LOOP
    BTFSC direction, 0
    GOTO DUTY_DOWN

DUTY_UP
    INCF    duty, F
    MOVLW   .30
    XORWF   duty, W
    BTFSC   STATUS, Z
    BSF     direction, 0     ; go DOWN
    GOTO    DUTY_OK

DUTY_DOWN
    DECF    duty, F
    BTFSC   STATUS, Z
    BCF     direction, 0     ; go UP

DUTY_OK
    CLRF l               ; reset ramp timing counters so ramp is smooth
    CLRF k

L_LOOP
	INCF    l, F	
	MOVLW   .3        ; value to compare against
	SUBWF   l, W       ; W = l - 10
	BTFSC   STATUS, Z   ; skip next if Z == 0
	GOTO    DUTY_CHANGE_LOOP

K_LOOP
	INCF    k, F	
	BTFSC   STATUS, Z   ; skip next if Z == 0
	GOTO    L_LOOP

    ; if (counter < duty) led_on else led_off

    MOVF  duty, W       ; W = duty
    SUBWF counter, W    ; W = counter - duty

    BTFSS STATUS, C     ; C = 0 ? counter < duty
    GOTO    DO_ON

	DO_OFF
	    BCF     GPIO, GP1
	    GOTO    AFTER_SET
	
	DO_ON
	    BSF     GPIO, GP1

AFTER_SET
    ; counter++
    INCF    counter, F   ; wraps automatically, no extra logic needed

    ; delay ˜ “step time” (sets PWM freq)
    CALL    DELAY

    GOTO    K_LOOP

; ~some small delay (tune N for desired freq)
DELAY
    MOVLW .10
    MOVWF j
D_LOOP
    DECFSZ j, F
    GOTO D_LOOP
    RETLW 0

    END
