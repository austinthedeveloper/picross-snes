# ADR-002: Version 1 is free play; story mode is cut
Status: Accepted (2026-11-07)

## Decision
The first SNES version has only the puzzles: free play over the web game's
523 pack puzzles in their 19 collections. Tracking, ranks, badges and
achievements stay, rebuilt around free play. Story mode is cut: jobs,
chapters, scenes and dialogue, fights (enemy HP, powers: fog, scramble,
swaps, forgeries, ink, charge, turns), gadgets, the path map, digs and
endless. The daily puzzle is cut too.

## Why
- The user asked for it: "Cut story mode. It's just the puzzles."
- Story mode is most of the web game's code and content (scenes, enemies,
  the battle rules) and would need its own art (portraits, backdrops) and
  a dialogue engine; free play needs none of it.
- The web game's badges were nearly all story badges (one per job, losses,
  fight mastery, endless depth, daily streaks). Without story mode they had
  to be replaced, not ported (ADR-004).
- A cartridge has no real-time clock, so a daily puzzle can't know the date.

## Rejected
- A daily puzzle from a counter ("puzzle of the session"): it isn't the
  same puzzle for everyone on the same day, which was the point.
- Keeping the web badges with story rules stubbed out: most could never be
  earned.

## Later
A version of the fights is in docs/IDEAS.md.

## Amendments
- 2026-11-07: murals are in after all (ADR-008): they're free-play
  puzzles, and as pack slots they cost little.
- 2026-11-07: story mode is coming in after all (ADR-010), imported from
  the web game and kept in step with it (ADR-011). The daily puzzle stays
  cut.
