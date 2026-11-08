.include "hdr.asm"

; void sram_copy_out(void) / void sram_copy_in(void)
;
; Copies sram_copy_len bytes (1-65535) between work RAM at sram_copy_wram
; (bank $7E) and battery-backed SRAM at offset sram_copy_sram (bank $70,
; LoROM) with MVN, about 7 cycles a byte (from Pickleball '94: the
; library's consoleCopySramWithOffset is several times slower). Inputs are
; C globals; the data bank (changed by MVN) is restored.

.SECTION ".sram_copy_text" SUPERFREE

sram_copy_out:
    php
    phb
    rep #$30
    lda.l sram_copy_wram
    tax
    lda.l sram_copy_sram
    tay
    lda.l sram_copy_len
    dec a
    mvn $7E,$70
    plb
    plp
    rtl

sram_copy_in:
    php
    phb
    rep #$30
    lda.l sram_copy_sram
    tax
    lda.l sram_copy_wram
    tay
    lda.l sram_copy_len
    dec a
    mvn $70,$7E
    plb
    plp
    rtl

.ENDS
