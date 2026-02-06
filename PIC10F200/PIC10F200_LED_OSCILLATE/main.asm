        LIST    P=10F200
        #include "p10f200.inc"
        __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_OFF

; ----------------------------
; Pin usage:
; GP0 = IN1
; GP1 = IN2
; GP2 = EN1
;
; L293D direction (EN=1):
; IN1=1 IN2=0  -> OUT1 high, OUT2 low
; IN1=0 IN2=1  -> OUT1 low,  OUT2 high
; ----------------------------

; RAM (file registers)
d1      EQU     0x10
d2      EQU     0x11

        ORG     0x0000

INIT:
        ; Make GP2 a normal GPIO (disable T0CKI function)
        MOVLW   ~(1<<T0CS)
        OPTION

        ; Set GP0, GP1, GP2 as outputs (TRIS bit = 0 => output)
        CLRW
        TRIS    GPIO

        ; Start in a known safe state: EN=0, IN1=0, IN2=0
        CLRF    GPIO

MAIN:
        ; -------- Polarity A: IN1=1, IN2=0 --------
        ; dead-time off first
        BCF     GPIO, GP2        ; EN=0
        CALL    DEADTIME_20US

        ; set direction
        BSF     GPIO, GP0        ; IN1=1
        BCF     GPIO, GP1        ; IN2=0

        ; enable
        BSF     GPIO, GP2        ; EN=1
        CALL    DELAY_2P5MS

        ; -------- Polarity B: IN1=0, IN2=1 --------
        BCF     GPIO, GP2        ; EN=0
        CALL    DEADTIME_20US

        BCF     GPIO, GP0        ; IN1=0
        BSF     GPIO, GP1        ; IN2=1

        BSF     GPIO, GP2        ; EN=1
        CALL    DELAY_2P5MS

        GOTO    MAIN


; ----------------------------
; Dead-time ~20 us @ 4 MHz (1 us per instruction cycle-ish)
; This is just to avoid nasty switching overlap.
; ----------------------------
DEADTIME_20US:
        ; 20 NOPs ~= 20 us
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        RETURN


; ----------------------------
; Delay ~2.5 ms @ 4 MHz
; Two nested loops.
; Adjust constants to tweak frequency.
; ----------------------------
DELAY_2P5MS:
        MOVLW   .10        ; outer loop count
        MOVWF   d1
D2P5_O:
        MOVLW   .10        ; inner loop count
        MOVWF   d2
D2P5_I:
        DECFSZ  d2, F
        GOTO    D2P5_I
        DECFSZ  d1, F
        GOTO    D2P5_O
        RETURN

        END
