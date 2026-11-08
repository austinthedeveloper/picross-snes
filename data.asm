.include "hdr.asm"

; Graphics (tools/gen_assets.py): loaded at boot.
.section ".rodata_gfx" superfree
font_pic_bin:
.incbin "assets/generated/font.pic"
font_pic_bin_end:
obj_pic_bin:
.incbin "assets/generated/obj.pic"
obj_pic_bin_end:
palette_pal_bin:
.incbin "assets/generated/palette.pal"
palette_pal_bin_end:
.ends

.section ".rodata_board" superfree
board_pic_bin:
.incbin "assets/generated/board.pic"
board_pic_bin_end:
.ends

; The puzzle pack (docs/PUZZLES.md).
.section ".rodata_pack_bits" superfree
pack_bits_bin:
.incbin "assets/generated/pack_bits.bin"
.ends

.section ".rodata_pack_index" superfree
pack_info_bin:
.incbin "assets/generated/pack_info.bin"
pack_names_bin:
.incbin "assets/generated/pack_names.bin"
pack_cinfo_bin:
.incbin "assets/generated/pack_cinfo.bin"
pack_cslots_bin:
.incbin "assets/generated/pack_cslots.bin"
pack_cnames_bin:
.incbin "assets/generated/pack_cnames.bin"
pack_minfo_bin:
.incbin "assets/generated/pack_minfo.bin"
.ends

; The story (tools/gen_story.py, ADR-010).
.include "assets/generated/story_data.asm"

; Portraits, enemy sprites and icons (tools/gen_art.py).
.include "assets/generated/art_data.asm"
