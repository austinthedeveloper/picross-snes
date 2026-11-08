# ADR-009: CI and releases
Status: Accepted (2026-11-07)

## Decision
GitHub Actions builds every PR and every push to `main`
(`.github/workflows/ci.yml`): `make check`, then `make release`, a check
that the ROM is 256 KiB, and the ROM and its symbols uploaded as an
artifact. Pushing a tag `vX.Y.Z` runs `.github/workflows/release.yml`,
which builds that commit the same way and publishes a GitHub release with
`picross-94-vX.Y.Z.sfc`, its `.sym` and a SHA-256, using
`docs/releases/vX.Y.Z.md` as the notes.

PVSnesLib is installed by `.github/actions/setup-pvsneslib`: the pinned
4.6.0 Linux release zip, unzipped, with `PVSNESLIB_HOME` pointing at it, as
its install guide says. It isn't cached; the download is 14 MB.

## Why
- The repo is public now; a PR should show it builds before anyone reads
  it, and a ROM to try without a toolchain.
- A release from a tag on CI is reproducible: the ROM is built from exactly
  the tagged commit, clean, not from someone's working tree.

## Details
- A release: bump `VERSION_NAME` in the Makefile, write
  `docs/releases/vX.Y.Z.md`, merge, then tag the merge commit on `main`
  and push the tag. The workflow fails if the tag doesn't match
  `VERSION_NAME` or the notes are missing.
- A failed release run can be rerun by hand (`workflow_dispatch` with the
  same tag); it builds the tagged commit and uploads to the existing
  release, so a tag never has to move.
- The ROM's build ID is the commit, so CI's ROM and a local `make release`
  of the same commit carry the same ID.

## Rejected
- Smoke runs on CI: MesenCE has no headless Linux package to install the
  documented way yet. They stay a local step (CLAUDE.md).
- Committing ROMs: build output stays out of git; releases carry them.

## Verification
The PR that added this ran `ci.yml` green; the first tag (v0.2.0)
published its release from `release.yml`.
