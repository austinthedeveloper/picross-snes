.include "hdr.asm"

; Shadow-map drawing in assembly (816-tcc spent about 230 cycles a stored
; entry in C, so a list row cost half a frame). Inputs are C globals; the
; maps are in bank $7E, addressed by md_dst (a word address's low 16 bits).
;
; void map_text(void)
;   Each character of the string at md_src (a C pointer, any bank) becomes
;   (char - 32) | md_attr, at most md_max of them, stopping at the NUL.
;   With md_pad set, pads with md_attr (a space) to md_max. md_count gets
;   the characters written before the NUL. The pointer is copied to the
;   compiler's scratch register tcc__r9 and read with [dp],y, as Pickleball
;   '94's hud_text_write does.
;
; void map_fill(void)
;   md_h rows of md_w entries (both nonzero) of md_entry from md_dst, the
;   map 32 entries wide.
;
; void map_hl(void)
;   The board's row/column highlight is palette bit 0 (2<->3, 4<->5, 6<->7,
;   docs/LAYOUT.md): sets (md_entry nonzero) or clears bit $0400 in md_w
;   entries from md_dst, md_h bytes apart (2 a row, 64 a column).

.SECTION ".map_draw_text" SUPERFREE

map_text:
    php
    phb
    rep #$30
    lda.l md_src
    sta.b tcc__r9
    lda.l md_src + 2
    sta.b tcc__r9h
    lda.l md_dst
    tax
    sep #$20
    lda.b #$7E
    pha
    plb
    rep #$20
    ldy.w #0
_mt_next:
    tya
    cmp.l md_max
    bcs _mt_full
    lda.b [tcc__r9],y
    and.w #$00FF
    beq _mt_end
    sec
    sbc.w #32
    ora.l md_attr
    sta.w $0000,x
    inx
    inx
    iny
    bra _mt_next
_mt_end:
    tya
    sta.l md_count
    lda.l md_pad
    beq _mt_out
_mt_pad:
    tya
    cmp.l md_max
    bcs _mt_out
    lda.l md_attr
    sta.w $0000,x
    inx
    inx
    iny
    bra _mt_pad
_mt_full:
    tya
    sta.l md_count
_mt_out:
    plb
    plp
    rtl

map_fill:
    php
    phb
    sep #$20
    lda.b #$7E
    pha
    plb
    rep #$30
    lda.l md_dst
    tax
    lda.l md_h
    sta.b tcc__r9
_mf_row:
    phx
    lda.l md_w
    tay
    lda.l md_entry
_mf_col:
    sta.w $0000,x
    inx
    inx
    dey
    bne _mf_col
    pla
    clc
    adc.w #64
    tax
    lda.l md_entry
    dec.b tcc__r9
    bne _mf_row
    plb
    plp
    rtl

map_hl:
    php
    phb
    sep #$20
    lda.b #$7E
    pha
    plb
    rep #$30
    lda.l md_dst
    tax
    lda.l md_w
    tay
    lda.l md_entry
    bne _mh_set
_mh_clear:
    lda.w $0000,x
    and.w #$FBFF
    sta.w $0000,x
    txa
    clc
    adc.l md_h
    tax
    dey
    bne _mh_clear
    bra _mh_out
_mh_set:
    lda.w $0000,x
    ora.w #$0400
    sta.w $0000,x
    txa
    clc
    adc.l md_h
    tax
    dey
    bne _mh_set
_mh_out:
    plb
    plp
    rtl

.ENDS
