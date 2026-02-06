        INCLUDE "p10f200.inc"
        __CONFIG _WDT_OFF & _CP_OFF & _MCLRE_ON

        ORG     0x0000

; -------------------------------------------------
; Registers
; -------------------------------------------------
d1          EQU     0x10        ; lower delay byte (for DELAY)
d2          EQU     0x11        ; upper delay byte (for DELAY)
periods     EQU     0x12        ; inner loop counter
duration    EQU     0x13        ; number of duration units

d1c         EQU     0x14        ; constant d1 for current note
d2c         EQU     0x15        ; constant d2 for current note
unitc       EQU 0x16    ; constant "Number of Periods" for current note
led_flag    EQU     0x17    ; toggles between 0 and 1

; -------------------------------------------------
; Durations (units)
; -------------------------------------------------
quarter_note      EQU    D'3'
half_note         EQU    D'6'
dotted_half_note  EQU    D'9'

; one “unit” of duration = this many half-periods
UNIT_PERIODS      EQU    D'85'   ; you can tweak tempo here

; -------------------------------------------------
; Init
; -------------------------------------------------
INIT
        MOVLW   ~(1<<T0CS)      ; internal instruction clock
        OPTION

	    MOVLW   b'00001000'   ; TRIS bits: 1=input, 0=output
	                          ; GP3=1 (input)
	                          ; GP2=0 (output)
	                          ; GP1=0 (output)
	                          ; GP0=0 (output)
	
	    TRIS    GPIO

; -------------------------------------------------
; MAIN
; -------------------------------------------------
MAIN_LOOP

        ; (1) E quarter_note
        MOVLW   quarter_note
        CALL    E1

        ; (2) A quarter_note
        MOVLW   quarter_note
        CALL    A1

		MOVLW   quarter_note
        CALL    C1

        MOVLW   quarter_note
        CALL    H1          ; B

        ; (5) A half, (6) E quarter
        MOVLW   half_note
        CALL    A1


        MOVLW   quarter_note
        CALL    E2

        ; (7) D dotted half
        MOVLW   dotted_half_note
        CALL    D2

        ; (8) B dotted half
        MOVLW   dotted_half_note
        CALL    H1


; -------------------------------------------------
; second line of Hedwig’s Theme only
; -------------------------------------------------
		
		MOVLW quarter_note
		CALL A1		

		MOVLW quarter_note
		CALL C1

		MOVLW quarter_note
		CALL H1

		MOVLW half_note
		CALL G1

		MOVLW quarter_note
		CALL A_SHARP1     ; B flat/bemol?

        ; First: E dotted half
        MOVLW  dotted_half_note
        CALL   E1        

		MOVLW quarter_note
		CALL   E1

        ; Fourth: E quarter (full)
        MOVLW  quarter_note
        CALL   E1

; -------------------------------------------------
; third line of Hedwig’s Theme only
; -------------------------------------------------

		MOVLW quarter_note
		CALL A1		

		MOVLW quarter_note
		CALL C1

		MOVLW quarter_note
		CALL H1

		MOVLW half_note
		CALL A1	

        MOVLW quarter_note
        CALL  E2

		MOVLW half_note
		CALL G2


		MOVLW quarter_note
		CALL F_SHARP2

		MOVLW half_note
		CALL F_SHARP2

        MOVLW   quarter_note
        CALL    C1_SHARP

; -------------------------------------------------
; forth line
; -------------------------------------------------

		MOVLW   quarter_note
        CALL    F2

		MOVLW   quarter_note
        CALL    E2

		MOVLW   quarter_note
        CALL    E_FLAT2

		MOVLW   half_note
        CALL    E_FLAT1
		
		MOVLW quarter_note
		CALL C1

		MOVLW dotted_half_note
		CALL A1

		MOVLW quarter_note
		CALL A1

		MOVLW quarter_note
		CALL C1

; -------------------------------------------------
; fifth line
; -------------------------------------------------

		MOVLW   half_note
        CALL    E2

		MOVLW quarter_note
		CALL C1

		MOVLW   half_note
        CALL    E2

		MOVLW quarter_note
		CALL C1

		MOVLW   half_note
        CALL    F2

		MOVLW   quarter_note
        CALL    E2

		MOVLW   half_note
        CALL    E_FLAT2

		MOVLW   quarter_note
        CALL    H1

		MOVLW quarter_note
		CALL C1

		MOVLW quarter_note
		CALL E2

		MOVLW quarter_note
		CALL E_FLAT2

		MOVLW half_note
		CALL E_FLAT1

		MOVLW quarter_note
		CALL E1

		MOVLW dotted_half_note
		CALL E2

		MOVLW half_note
		CALL E2

		MOVLW quarter_note
		CALL C1


; next line

		MOVLW half_note
		CALL E2

		MOVLW quarter_note
		CALL C1

		MOVLW half_note
		CALL E2

		MOVLW quarter_note
		CALL C1



		SLEEP

        GOTO    MAIN_LOOP


; ------------------------------------------
; TOGGLE_LED — flip between GP0 and GP1
; ------------------------------------------
TOGGLE_LED
        ; flip bit0 of led_flag: 0?1
        MOVF    led_flag, W
        XORLW   1
        MOVWF   led_flag

        ; clear both LEDs
        BCF     GPIO, GP0
        BCF     GPIO, GP1

        ; if bit0 == 1 ? LED1 on, else LED0 on
        BTFSC   led_flag, 0      ; if bit0 = 1
        BSF     GPIO, GP1        ;   turn on LED1

        BTFSS   led_flag, 0      ; if bit0 = 0
        BSF     GPIO, GP0        ;   turn on LED0

        RETLW   0

; -------------------------------------------------
; NOTE BODY (shared by all notes)
;   - W = duration units on entry to stub
;   - stub stores W in 'duration' and sets d1c/d2c
; -------------------------------------------------
NOTE_BODY
		CALL    TOGGLE_LED
NOTE_UNIT_LOOP
        MOVF    unitc, W       ; load per-note Number of Periods
        MOVWF   periods

NOTE_PERIOD_LOOP
        MOVLW   (1<<GP2)
        XORWF   GPIO, F

        ; load d1/d2 from per-note constants
        MOVF    d1c, W
        MOVWF   d1
        MOVF    d2c, W
        MOVWF   d2
        CALL    DELAY

        DECFSZ  periods, F
        GOTO    NOTE_PERIOD_LOOP

        DECFSZ  duration, F    ; duration = how many times we repeat that block
        GOTO    NOTE_UNIT_LOOP

        RETLW   0



; -------------------------------------------------
; NOTE STUBS – only set duration + d1c/d2c, then jump to NOTE_BODY
; -------------------------------------------------

; ========== REDUCED NOTE STUBS (safe timing) ==========

; ---------- F#2 ----------
; F#2: d1=224, d2=1, Number of Periods=191
F_SHARP2
        MOVWF   duration
        MOVLW   D'224'
        MOVWF   d1c
        MOVLW   D'1'
        MOVWF   d2c
        MOVLW   D'191'
        MOVWF   unitc
        GOTO    NOTE_BODY

; G2: d1=210, d2=1, Number of Periods=202
G2
        MOVWF   duration
        MOVLW   D'210'
        MOVWF   d1c
        MOVLW   D'1'
        MOVWF   d2c
        MOVLW   D'202'
        MOVWF   unitc
        GOTO    NOTE_BODY

; E1: d1=248, d2=2, Number of Periods=85
E1
        MOVWF   duration
        MOVLW   D'248'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'85'
        MOVWF   unitc
        GOTO    NOTE_BODY

; E2: d1=251, d2=1, Number of Periods=170
E2
        MOVWF   duration
        MOVLW   D'251'
        MOVWF   d1c
        MOVLW   D'1'
        MOVWF   d2c
        MOVLW   D'170'
        MOVWF   unitc
        GOTO    NOTE_BODY

; A1: d1=121, d2=2, Number of Periods=114
A1
        MOVWF   duration
        MOVLW   D'121'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'114'
        MOVWF   unitc
        GOTO    NOTE_BODY

; C1: d1=123, d2=3, Number of Periods=68
C1
        MOVWF   duration
        MOVLW   D'61'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'135'
        MOVWF   unitc
        GOTO    NOTE_BODY

; C#1: d1=88, d2=3, Number of Periods=72
C1_SHARP
        MOVWF   duration
        MOVLW   D'43'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'143'
        MOVWF   unitc
        GOTO    NOTE_BODY

; H1/B1: d1=80, d2=2, Number of Periods=128
H1
        MOVWF   duration
        MOVLW   D'80'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'128'
        MOVWF   unitc
        GOTO    NOTE_BODY

; D2: d1=26, d2=2, Number of Periods=152
D2
        MOVWF   duration
        MOVLW   D'26'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'152'
        MOVWF   unitc
        GOTO    NOTE_BODY

; G1: d1=168, d2=2, Number of Periods=101
G1
        MOVWF   duration
        MOVLW   D'168'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'101'
        MOVWF   unitc
        GOTO    NOTE_BODY

; A#1: d1=100, d2=2, Number of Periods=120
A_SHARP1
        MOVWF   duration
        MOVLW   D'100'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'120'
        MOVWF   unitc
        GOTO    NOTE_BODY

; ---------- F2 ----------
; Table: d1=237, d2=1, Number of Periods=180
F2
        MOVWF   duration        ; W = duration multiplier (quarter/half/etc.)
        MOVLW   D'237'
        MOVWF   d1c
        MOVLW   D'1'
        MOVWF   d2c
        MOVLW   D'180'
        MOVWF   unitc
        GOTO    NOTE_BODY


; ---------- Eb2 / D#2 ----------
; Table: d1=10, d2=2, Number of Periods=160
E_FLAT2
        MOVWF   duration
        MOVLW   D'10'
        MOVWF   d1c
        MOVLW   D'2'
        MOVWF   d2c
        MOVLW   D'160'
        MOVWF   unitc
        GOTO    NOTE_BODY


; ---------- Eb1 / D#1 ----------
; Table: d1=22, d2=3, Number of Periods=80
E_FLAT1
        MOVWF   duration
        MOVLW   D'22'
        MOVWF   d1c
        MOVLW   D'3'
        MOVWF   d2c
        MOVLW   D'80'
        MOVWF   unitc
        GOTO    NOTE_BODY


; -------------------------------------------------
; DELAY subroutine – uses d1, d2
; -------------------------------------------------
DELAY
        DECFSZ  d1, F
        GOTO    DELAY
        DECFSZ  d2, F
        GOTO    DELAY
        RETLW   0

        END
