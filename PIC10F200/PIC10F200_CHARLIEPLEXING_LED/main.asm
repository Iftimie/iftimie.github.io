#include "p10f200.inc"
    __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF
    ORG 0x0000
i   	 EQU   	 10   	 ;define 0x10 register as the delay variable
j   	 EQU   	 11   	 ;define 0x11 register as the delay variable
k   	 EQU   	 12   	 ;define 0x12 register as the delay variable

INIT
    MOVLW  ~(1<<T0CS) 	 ;Enable GPIO2
    OPTION    
    MOVLW ((1 << GP0)|(1 << GP1)|(1 << GP2));set GP0, GP1, GP2 as inputs
    TRIS GPIO
LOOP
	MOVLW ((0 << GP0)|(0 << GP1)|(1 << GP2))
	TRIS GPIO
	MOVLW (1 << GP0) | (0 << GP1)
	MOVWF GPIO

	CALL DELAY

	MOVLW ((0 << GP0)|(0 << GP1)|(1 << GP2))
	TRIS GPIO
	MOVLW (0 << GP0) | (1 << GP1)
	MOVWF GPIO

	CALL DELAY

	MOVLW ((1 << GP0)|(0 << GP1)|(0 << GP2))
	TRIS GPIO
	MOVLW (1 << GP1) | (0 << GP2)
	MOVWF GPIO

	CALL DELAY

	MOVLW ((1 << GP0)|(0 << GP1)|(0 << GP2))
	TRIS GPIO
	MOVLW (0 << GP1) | (1 << GP2)
	MOVWF GPIO

	CALL DELAY

	MOVLW ((0 << GP0)|(1 << GP1)|(0 << GP2))
	TRIS GPIO
	MOVLW (1 << GP0) | (0 << GP2)
	MOVWF GPIO

	CALL DELAY

	MOVLW ((0 << GP0)|(1 << GP1)|(0 << GP2))
	TRIS GPIO
	MOVLW (0 << GP0) | (1 << GP2)
	MOVWF GPIO

	CALL DELAY
    
    GOTO LOOP     		 ;loop forever
 
DELAY   			 ;Start DELAY subroutine here
    MOVLW 6   			 ;Load initial value for the delay    
    MOVWF i   			 ;Copy the value to the register 0x10
    MOVWF j   			 ;Copy the value to the register 0x11
    MOVWF k   			 ;Copy the value to the register 0x12
DELAY_LOOP   		 ;Start delay loop
    DECFSZ i, F   		 ;Decrement the register i and check if not zero
    GOTO DELAY_LOOP   	 ;If not then go to the DELAY_LOOP label
    DECFSZ j, F   		 ;Else decrement the register j, check if it is not 0
    GOTO DELAY_LOOP   	 ;If not then go to the DELAY_LOOP label
    DECFSZ k, F   		 ;Else decrement the register k, check if it is not 0
    GOTO DELAY_LOOP   	 ;If not then go to the DELAY_LOOP label
    RETLW 0   			 ;Else return from the subroutine

    END