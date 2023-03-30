/*
 *
 *  Author: Thomas Creel
 *  Purpose: To allow LED animations to be created
 *
 *    s1 MB EDIT MODE on depress [not debounced]
 *    s2 MB PLAY MODE on depress [not debounced]
 *
 *    s1 SLB STORE_DIP_SWITCH on depress [debounced]
 *    s2 SLB N/A
 *
 *    s1 PAD N/A
 *
 *    starts in EDIT mode (LEDs on SLB continually updated with current values of DIP switches on SLB)
 *    if s1 SLB released the current values of DIP switches are stored into 8-bit data memory table once, starting at 0x2000
 *    if s2 MB is pressed PLAY MODE starts (data memory starting at 0x2000 is stored into LED, iterate through data memory every 20Hz using timer/counter, at end of animation, restart)
 *    if s1 MB is pressed EDIT MODE starts (data memory remains unchanged, next frame stored at end of last frame)
 *
 */ 
 

;*********************************INCLUDES*************************************


; include the assembler. 
.include "ATxmega128a1udef.inc"
;******************************END OF INCLUDES*********************************

;******************************DEFINED SYMBOLS*********************************
.equ ANIMATION_START_ADDR   =   0x2000
.equ ANIMATION_SIZE         =   (0x3FFF - ANIMATION_START_ADDR)
;**************************END OF DEFINED SYMBOLS******************************

;******************************MEMORY CONSTANTS********************************
; data memory allocation
.dseg

.org ANIMATION_START_ADDR
ANIMATION:
.byte ANIMATION_SIZE
;***************************END OF MEMORY CONSTANTS****************************

;********************************MAIN PROGRAM**********************************
.cseg
; upon system reset, jump to main program (instead of executing
; instructions meant for interrupt vectors)
.org 0x0
    rjmp MAIN

; place the main program somewhere after interrupt vectors (ignore for now)
.org 0x100      ; >= 0xFD
MAIN:
; initialize the stack pointer to max SRAM address 0x3FFF
    ldi r16, 0xFF
    sts CPU_SPL, r16
    ldi r16, 0x3F
    sts CPU_SPH, r16
; initialize relevant I/O modules (switches and LEDs)
    rcall IO_INIT

; initialize (but do not start) the relevant timer/counter module(s)
    rcall TC_INIT

; Initialize the X and Y indices to point to the beginning of the 
; animation table. (Although one pointer could be used to both
; store frames and playback the current animation, it is simpler
; to utilize a separate index for each of these operations.)
; Note: recognize that the animation table is in DATA memory
    ldi XL, byte1(ANIMATION)  ; always points to 0x2000
    ldi XH, byte2(ANIMATION) ; X = 0x2000

    ldi YL, byte1(ANIMATION) ; starts at 0x2000, always points to next available data memory space
    ldi YH, byte2(ANIMATION) ; Y = 0x2000



; begin main program loop 
    
; "EDIT" mode
EDIT:
    
; Check if it is intended that "PLAY" mode be started, i.e.,
; determine if the relevant switch has been pressed.
; PLAY button = MB S2 = PORTE bit 0 [not debounced]

    ; check if s2 MB is pressed (PORTE bit 0)
    ; read portE input
    lds r16, PORTE_IN

    ; check if PORTE bit 0 is pressed
    sbrs r16, 0 ; skip if bit in register set
    rjmp PLAY_PRESSED ; PORTE_IN = 0
    rjmp PLAY_NOT_PRESSED ; PORTE_IN = 1


PLAY_PRESSED:
; If it is determined that relevant switch was pressed, 
; go to "PLAY" mode.
    rjmp PLAY
    

PLAY_NOT_PRESSED:
; Otherwise, if the "PLAY" mode switch was not pressed,
; update display LEDs with the voltage values from relevant DIP switches
; and check if it is intended that a frame be stored in the animation
; (determine if this relevant switch has been pressed). (s1 SLB STORE_DIP_SWITCH on depress [debounced]  --> PORT F2 bit 2)
    ; read portA input to display LEDS
    lds r16, PORTA_IN 

    ; store into PORTC output
    sts PORTC_OUT, r16

    ; PORTF bit 2 = switch SLB S1
    lds r16, PORTF_IN

    ; check if PORTF bit 2 is pressed
    sbrs r16, 2 ; skip if bit in register set
    rjmp DEBOUNCE ; PORTF bit 2 = 0 (BUTTON IS PRESSED)
    rjmp EDIT ; PORTF bit 2 = 1 (BUTTON IS NOT PRESSED); If the "STORE_FRAME" switch was not pressed, ; i.e. s1 slb  branch back to "EDIT".

    

; Otherwise, if it was determined that relevant switch was pressed,
; perform debouncing process, e.g., start relevant timer/counter
; and wait for it to overflow. (Write to CTRLA and loop until
; the OVFIF flag within INTFLAGS is set.)
DEBOUNCE: 
    ; uses Timer/Counter 1
    ; 0.000015 second debounce timer period 
    lds r16, TCC1_INTFLAGS
    sbrc r16, 0 ; skip if bit in register clear
    rjmp TCC1_OVFIF_DEBOUNCE ; OVFIF = 1
    rjmp DEBOUNCE ; OVFIF = 0

; After relevant timer/counter has overflowed (i.e., after
; the relevant debounce period), disable this timer/counter,
; clear the relevant timer/counter OVFIF flag,
; and then read switch value again to verify that it was
; actually pressed. If so, perform intended functionality, and
; otherwise, do not; however, in both cases, wait for switch to
; be released before jumping back to "EDIT".
TCC1_OVFIF_DEBOUNCE:
    
    sbr r16, 0 ;set bit0 = 1 ; clear intflag bit0 by setting it to 1
    sts TCC1_INTFLAGS, r16 ; storing new value, this clears it
      
    ; read store_frame button value again (portf bit 2)
    lds r16, PORTF_IN 

    ; check if PORTF bit 2 is pressed
    sbrs r16, 2 ;  skip if bit in register set
    rjmp SWITCH_DEPRESSED ; PORTF bit 2 = 0 (depressed)
    rjmp EDIT ; PORTF bit 2 = 1 (untouched) ; If the "STORE_FRAME" switch was not pressed, ; i.e. s1 slb  branch back to "EDIT".

; Wait for the "STORE FRAME" switch to be released before jumping to "EDIT".
SWITCH_DEPRESSED:
    ; only execute when PORTF_IN bit 2 = 1
    lds r16, PORTF_IN
    sbrc r16, 2 ; skip if bit in register cleared
    rjmp STORE_FRAME        ; r16 = 1 (i.e. release occurred) 
    rjmp SWITCH_DEPRESSED   ; r16 = 0 (i.e still pressing)

STORE_FRAME:
    ; read portA input to display LEDS
    lds r16, PORTA_IN 

    ; store into data memory and increment Y
    st Y+, r16 
    rjmp EDIT

    
; "PLAY" mode
PLAY:

; Reload the relevant index to the first memory location
; within the animation table to play animation from first frame.




PLAY_LOOP:

; Check if it is intended that "EDIT" mode be started ; s1 MB EDIT MODE MB 10-E1 = S2 connect PORTE bit 1 [not debounced]
; i.e., check if the relevant switch has been pressed.`
    ; read PORTE 
    lds r16, PORTE_IN

    sbrs r16, 1 ; skip if bit in register set ; check if PORTE bit 1 is pressed
    rjmp EDIT_PRESSED ; PORTE_IN = 0 (PRESSED)
    rjmp EDIT_NOT_PRESSED ; PORTE_IN = 1 (NOT PRESSED)

; If it is determined that relevant switch was pressed, 
; go to "EDIT" mode.
EDIT_PRESSED:
    rjmp EDIT

; Otherwise, if the "EDIT" mode switch was not pressed,
; determine if index used to load frames has the same
; address as the index used to store frames, i.e., if the end
; of the animation has been reached during playback.
; (Placing this check here will allow animations of all sizes,
; including zero, to playback properly.)
; To efficiently determine if these index values are equal,
; a combination of the "CP" and "CPC" instructions is recommended.
EDIT_NOT_PRESSED:

    ; address used to load frames Y (points at end) (load)
    ; address used to store frames X (points at beginning) (store)
    cp XL, YL
    breq EQUAL_LOWS
    jmp NOT_EQUAL
    
EQUAL_LOWS:
    cp XH, YH
    breq EQUAL_XY
    jmp NOT_EQUAL


; If index values are equal, branch back to "PLAY" to
; restart the animation.
EQUAL_XY:
    rjmp PLAY

; Otherwise, load animation frame from table, 
; display this "frame" on the relevant LEDs,
; start relevant timer/counter,
; wait until this timer/counter overflows (to more or less
; achieve the "frame rate"), and then after the overflow,
; stop the timer/counter,
; clear the relevant OVFIF flag,
; and then jump back to "PLAY_LOOP".
NOT_EQUAL:

ANIMATION_LOOP:
    ; load animation frame from table, X starts at 0x2000, increment X
    ld r16, X+

    ; store frame into LED
    lds r16, PORTA_IN 

    ; store into PORTC output
    sts PORTC_OUT, r16

    ; start 20hz counter timer 0
CNTR20HZ:
    lds r16, TCC0_INTFLAGS ; check bit
    sbrc r17, 0 ; skip if bit in register cleared
    rjmp TIMERDONE ; r17 = 1 (i.e. OVFIF set)
    rjmp CNTR20HZ ; r17 = 0 (i.e. OVFIF not set)

TIMERDONE:
    
    sbr r17, 0 ; clear intflag bit0 by setting it to 1
    sts TCC0_INTFLAGS, r17 ; storing new value clears it

    ; jump back to play
    rjmp PLAY_LOOP



; end of program (never reached)
DONE: 
    rjmp DONE
;*****************************END OF MAIN PROGRAM *****************************

;********************************SUBROUTINES***********************************

;******************************************************************************
; Name: IO_INIT 
; Purpose: To initialize the relevant input/output modules, as pertains to the
;          application.
; Input(s): N/A
; Output: N/A
;******************************************************************************
IO_INIT:
; protect relevant registers
    push r16
    clr r16

; initialize the relevant I/O
    ; initialize output LED (portC) 
    ldi r16, 0xFF 
    sts PORTC_OUT, r16

    ; set output LED direction (portC) 
    sts PORTC_DIR, r16

    ; set input switches direction (portA) 
    ldi r16, 0x00
    sts PORTA_DIR, r16

; recover relevant registers
    pop r16
; return from subroutine
    ret
;******************************************************************************
; Name: TC_INIT 
; Purpose: To initialize the relevant timer/counter modules, as pertains to
;          application.
; Input(s): N/A
; Output: N/A
;******************************************************************************
TC_INIT:
; protect relevant registers
    push r16

; initialize the relevant TC modules
    ; setup timer 0 (50ms period)
    ; configure PER timer 0 = 0xC9BC
    ldi r16, 0xBC
    sts TCC0_PER, r16
    ldi r16, 0xC9
    sts TCC0_PER + 1, r16

    ; configure prescale = 2 = 0010
    ldi r16, 0b00000010
    sts TCC0_CTRLA, r16

    ; setup timer 1 (0.015ms period)
    ; configure PER timer 1 = 0x001E 
    ldi r16, 0x1E
    sts TCC1_PER, r16
    ldi r16, 0x00
    sts TCC1_PER + 1, r16

    ; configure prescale timer 1 = 1 = 0001
    ldi r16, 0b00000001
    sts TCC1_CTRLA, r16

; recover relevant registers
    pop r16
; return from subroutine
    ret

;*****************************END OF SUBROUTINES*******************************
