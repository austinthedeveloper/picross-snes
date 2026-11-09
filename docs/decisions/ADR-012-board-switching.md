# ADR-012: Bosses that switch board play flat
Status: Accepted (2026-11-08)

## Decision
The web game's boss phases can carry `"board": "block"` or `"board": "flat"`
(web PR #136): the fight moves onto a 3D block (Picross Cubed's) and back.
The SNES has no block, so the story import accepts the field and drops it
(`tools/import_story.py`), and the whole fight plays on the flat board.

The phase itself still imports: its HP threshold, name and taunt show as an
ordinary phase card. The Curator is the first such boss: at 60% it becomes
"Unfolded Curator" ("Flat was never my best side.") and at 30% "Overwound
Curator" again, both flat here.

## Why
- The switch is the web's way of mixing its two games; the fight's rules
  (HP, chip damage, phases, powers) are the same on either board, so a flat
  fight is a faithful SNES version of it, as Cubed is a faithful 3D one.
- Failing the import ("needs porting") would stop every web sync until a 3D
  board exists, which it never will on this cartridge.
- A switching phase can't swap or splash ink on the web (they'd land on the
  board that's going), so dropping `board` never leaves a phase doing
  something the web wouldn't.

## Details
- On the web, a block has more lines and so more HP, carried as a share;
  flat, the HP never changes scale, so nothing carries.
- The phase name reads oddly flat ("Unfolded" on a flat board). It's the
  web's text, imported as is; if it grates, the fix is on the web side (a
  name that reads on both), not an SNES override.
