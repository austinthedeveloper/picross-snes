.include "hdr.asm"

; void wram_move(void)
;
; Moves wm_len bytes (nonzero) within bank $7E from wm_src to wm_dst,
; overlapping either way: MVN when moving down in memory, MVP (from the
; end) when moving up. Scrolling a list shifts its map rows with this
; instead of redrawing every row (KNOWN_ISSUES #4).

.SECTION ".wram_move_text" SUPERFREE

wram_move:
    php
    phb
    rep #$30
    lda.l wm_dst
    cmp.l wm_src
    bcs _wm_up
    lda.l wm_src
    tax
    lda.l wm_dst
    tay
    lda.l wm_len
    dec a
    mvn $7E,$7E
    bra _wm_out
_wm_up:
    lda.l wm_src
    clc
    adc.l wm_len
    dec a
    tax
    lda.l wm_dst
    clc
    adc.l wm_len
    dec a
    tay
    lda.l wm_len
    dec a
    mvp $7E,$7E
_wm_out:
    plb
    plp
    rtl

.ENDS
