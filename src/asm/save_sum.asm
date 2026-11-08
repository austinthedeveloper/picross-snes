.include "hdr.asm"

; void save_sum(void)
;
; The save checksum (src/save.c, save_checksum, is the reference the host
; tests run): sum = $1994; for each of save_sum_len bytes at save_sum_src
; (bank $7E) except offsets 4 and 5, sum = rol16(sum) + byte. Result in
; save_sum_acc. In 816-tcc C this took 17 frames for the 2576-byte save;
; here it takes about two.

.SECTION ".save_sum_text" SUPERFREE

save_sum:
    php
    phb
    sep #$20
    lda.b #$7E
    pha
    plb
    rep #$30
    lda.l save_sum_src
    tax
    lda.w #$1994
    sta.l save_sum_acc
    ldy.w #0
_ss_loop:
    cpy.w #4
    beq _ss_skip
    cpy.w #5
    beq _ss_skip
    lda.l save_sum_acc
    asl a
    adc.w #0
    sta.l save_sum_acc
    sep #$20
    lda.w $0000,x
    rep #$20
    and.w #$00FF
    clc
    adc.l save_sum_acc
    sta.l save_sum_acc
_ss_skip:
    inx
    iny
    tya
    cmp.l save_sum_len
    bne _ss_loop
    plb
    plp
    rtl

.ENDS
