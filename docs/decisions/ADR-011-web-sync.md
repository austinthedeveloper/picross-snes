# ADR-011: Keeping in step with the web game
Status: Accepted (2026-11-07)

## Decision
`.github/workflows/sync-web.yml` runs `make import` against the web game's
`main` every day, on demand (Actions → Sync from the web game → Run), and
when the web repo sends a `web-updated` repository dispatch:

- **Nothing changed:** it stops.
- **It imports and builds:** it commits the data to the branch `sync/web`
  and opens (or updates) one PR, "Sync from the web game", saying what
  changed (`tools/sync_summary.py`: new and changed puzzles, murals,
  chapters, jobs, fights, scenes, art, tuning) with a release ROM to try.
- **It needs porting:** the import's report (ADR-010) goes into one issue,
  "Web sync needs porting", and nothing else changes. The issue closes
  itself once a sync succeeds.

A web update reaches the cartridge by merging a PR. Porting a new mechanic
is ordinary work: add the code, add it to `SUPPORTED`, and the next sync
goes through.

## Why
- The user asked for "some sort of pipeline to easily update this snes game
  when it's time". Most web updates are content, which this makes a merge.
- One rolling branch and PR, not one per day: the PR always shows
  everything the cartridge is behind by.
- The gate's report as an issue says exactly what to port, in the place
  work is tracked.

## Details
- The sync job runs `make check` and `make release` itself, because a PR
  opened with the workflow's token doesn't start CI (a GitHub rule against
  workflow loops). Closing and reopening the PR runs CI if wanted.
- For the workflow to open PRs, the repo needs **Settings → Actions →
  General → Allow GitHub Actions to create and approve pull requests**.
- The web repo can ping it on push (`.github/workflows/notify-snes.yml`
  there) with a `repository_dispatch`, which needs a token with access to
  this repo saved as the web repo's `SNES_SYNC_TOKEN` secret. Without it,
  the daily run picks changes up within a day.
- Locally: `make import` (the sibling checkout) or `make import WEB=<path>`.

## Rejected
- Git submodule of the web repo: a submodule bump is a sync with none of
  the checks or the summary.
- Auto-merging the sync PR: new content deserves a play first, and the ROM
  is right there.
