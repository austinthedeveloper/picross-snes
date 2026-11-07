.include "hdr.asm"

; void map_commit(void)
;
; In VBlank: uploads the dirty 8-row bands of the BG1 and BG3 shadow maps
; (g_bg1_map / g_bg3_map, bit n of g_bg1_dirty / g_bg3_dirty for band n,
; 512 bytes each) to VRAM, at most MAX_BANDS a frame; the rest stay dirty
; for the next frame. DMA channel 0, written directly (dmaCopyVram's call
; overhead was about four scanlines a band in Pickleball '94). The NMI has
; already sent OAM when this runs.

.DEFINE MAX_BANDS 5
.DEFINE VRAM_BG1_MAP $5000
.DEFINE VRAM_BG3_MAP $5400

; \1 map, \2 dirty byte, \3 VRAM word address, \4 band, \5 bit, \6 ~bit
.MACRO COMMIT_BAND
    lda.l \2
    and.b #\5
    beq +
    cpy.w #MAX_BANDS
    beq +
    rep #$20
    lda.w #(\3 + \4 * 256)
    sta.l $2116
    lda.w #((\1 + \4 * 512) & $FFFF)
    sta.l $4302
    lda.w #512
    sta.l $4305
    sep #$20
    lda.b #:\1
    sta.l $4304
    lda.b #$01
    sta.l $420B
    lda.l \2
    and.b #\6
    sta.l \2
    iny
+
.ENDM

.SECTION ".map_commit_text" SUPERFREE

map_commit:
    php
    sep #$20
    rep #$10
    lda.l g_bg1_dirty
    ora.l g_bg3_dirty
    bne _mc_go
    jmp _mc_done
_mc_go:
    lda.b #$80                  ; VMAIN: word writes, step after the high byte
    sta.l $2115
    lda.b #$01                  ; DMA mode 1: $2118/$2119
    sta.l $4300
    lda.b #$18
    sta.l $4301
    ldy.w #0                    ; bands sent
    COMMIT_BAND g_bg3_map, g_bg3_dirty, VRAM_BG3_MAP, 0, $01, $FE
    COMMIT_BAND g_bg3_map, g_bg3_dirty, VRAM_BG3_MAP, 1, $02, $FD
    COMMIT_BAND g_bg3_map, g_bg3_dirty, VRAM_BG3_MAP, 2, $04, $FB
    COMMIT_BAND g_bg3_map, g_bg3_dirty, VRAM_BG3_MAP, 3, $08, $F7
    COMMIT_BAND g_bg1_map, g_bg1_dirty, VRAM_BG1_MAP, 0, $01, $FE
    COMMIT_BAND g_bg1_map, g_bg1_dirty, VRAM_BG1_MAP, 1, $02, $FD
    COMMIT_BAND g_bg1_map, g_bg1_dirty, VRAM_BG1_MAP, 2, $04, $FB
    COMMIT_BAND g_bg1_map, g_bg1_dirty, VRAM_BG1_MAP, 3, $08, $F7
_mc_done:
    plp
    rtl

.ENDS
