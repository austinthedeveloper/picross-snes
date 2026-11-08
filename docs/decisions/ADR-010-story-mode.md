# ADR-010: Story mode, imported from the web game
Status: Accepted (2026-11-07)

## Decision
Port the web game's story mode: chapters of jobs, each a run of fights,
digs and rest stops; enemies with HP, attacks, phases and the eight powers
(fog, scramble, armor, fakes, charge, ink, swap, rotate); gadgets; scenes
with portraits, backdrops, choices and flags; then Overtime, boss fights,
Endless and the story badges.

The web game stays the only place the story is written. It isn't finished
and keeps growing, so the SNES copies it as data rather than as a rewrite:

- `tools/import_story.py` reads the web repo's chapters, runs, encounters,
  scenes, backdrops and story-only puzzles, the gadget catalog and power
  tips (TypeScript literals), and the tuning constants (`CHIP_DAMAGE`,
  `ZAP_DAMAGE`, ...), and writes one committed `data/story.json`.
- `tools/import_art.py` turns the web art (cast portraits, enemy sprites,
  gadget and node icons, power glyphs) into small indexed images in
  `data/art/`, so the ROM build never needs the web repo.
- `tools/gen_assets.py` turns those into ROM tables. Layout decisions (line
  wrapping, glyph codes, tile sizes) live there, not in the importer, so the
  import stays a faithful copy.

The importer is the compatibility gate. It knows exactly which fields,
powers, gadget effects, encounter kinds and characters the SNES has code
for (`SUPPORTED`, `EMOJI`). Anything else fails the import, writes nothing,
and lists what needs porting. A new web mechanic can't reach the cartridge
unplayed, and nobody has to diff the web repo by hand to find it.

The rules port is a line-for-line translation of `src/game/battle.ts`,
`run.ts`, `dialogue.ts` and the engine helpers (`swap.ts`, `fakes.ts`,
`ink.ts`, `charge.ts`, `blast.ts`, `transform.ts`), with host tests that
mirror the web tests, so the two games play the same fight the same way.

## Why
- The user wants the story in, and the web game is "the go-to for future
  updates": the SNES follows it, it never leads.
- Data plus a gate keeps the port honest as the web changes: text, new
  chapters, new jobs and fights built from known powers flow through with
  no SNES code; a new power stops the line until it's ported.
- Constants come across too, so a web rebalance (a zap costing 3) needs no
  SNES change.

## Details
- Save slots stay append-only (ADR-003): `jobSlots` and `sceneSlots` in
  `data/story.json` give jobs and scenes a slot forever, for job results
  and seen scenes in the battery save.
- Text is ASCII. Emoji become tokens (`<mask>`, `<bolt>`) that the ROM build
  draws with its own glyphs; curly quotes and dashes become ASCII.
- Scene flags get a bit each, `won` first; the SNES keeps 8.
- Story puzzles (`src/puzzles/run/`) are ROM puzzles after the pack, not
  save slots: they're never free play.
- The ROM grows (512 KiB) for the portraits, sprites and text.

## Rejected
- Writing the story again for the SNES: two scripts drift, and the web one
  is the one people edit and review.
- Importing straight from the web repo at build time: the ROM build would
  need the web repo, and a web change could break a SNES build silently.
- Silently skipping unknown fields: a fight without its new power is a
  different fight.

## Verification
`python3 tools/import_story.py` imports the whole web story. With a made-up
enemy field, phase field, gadget effect and emoji in a copy of the web
source, it fails listing all four and leaves `data/story.json` untouched.
