#!/usr/bin/env python3
"""Runs smoke timelines headless and tiles their screenshots.

  python3 tools/smoke.py NAME [NAME ...]     some timelines
  python3 tools/smoke.py --all               every test/smoke/*.timeline
  python3 tools/smoke.py --list              names and their lengths

Options:
  --rom PATH       play this ROM instead of building the debug ROM (a CI
                   artifact, a release); its .sym next to it serves pokes
  --release TAG    download that GitHub release's ROM and play it
  --run ID         download that CI run's ROM artifact and play it
  --frames N       run N frames (default: the timeline's last frame + 40)
  --keep-save      keep the battery save between runs (default: blank)
  --probes SPEC    SMOKE_PROBES, e.g. "hp=1f2a:1" (CLAUDE.md)
  --no-build       use the debug ROM as it is

Each run writes build/smoke/NAME/frame-NNNNN.png and a contact sheet
build/smoke/NAME.png; with several runs, build/smoke/all.png tiles the last
screenshot of each. A test/smoke/NAME.pokes file sets RAM up first. A run
that writes fewer screenshots than its timeline asks for is a failure: the
summary says which, and the exit code is 1.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import contact_sheet  # noqa: E402
from pokes import smoke_pokes  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SMOKE = ROOT / "test" / "smoke"
OUT = ROOT / "build" / "smoke"
MESEN = os.environ.get("MESEN_BIN", "/Applications/Mesen.app/Contents/MacOS/Mesen")
SAVES = Path(os.environ.get("MESEN_SAVES",
                            Path.home() / "Library/Application Support/MesenCE/Saves"))


def timeline_info(path):
    """(last frame, shot frames) of a timeline."""
    last, shots = 0, []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#")[0].split()
        if not line:
            continue
        if line[0] == "shot":
            shots.append(int(line[1]))
            last = max(last, int(line[1]))
        elif re.fullmatch(r"\d+", line[0]):
            last = max(last, int(line[1]) if len(line) > 1 and line[1].isdigit() else int(line[0]))
    return last, shots


def build_debug():
    r = subprocess.run(["make", "debug"], cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:], sep="\n")
        sys.exit("make debug failed")
    roms = sorted((ROOT / "build/debug").glob("picross-*.sfc"))
    if not roms:
        sys.exit("make debug built no ROM")
    return roms[-1]


def download(kind, ident):
    dest = ROOT / "build" / "downloads" / f"{kind}-{ident}"
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True)
    repo = ["-R", "austinthedeveloper/picross-snes"]
    if kind == "release":
        cmd = ["gh", "release", "download", ident, *repo, "-D", str(dest), "-p", "*.sfc", "-p", "*.sym"]
    else:
        cmd = ["gh", "run", "download", ident, *repo, "-D", str(dest)]
    subprocess.run(cmd, check=True)
    roms = sorted(dest.rglob("*.sfc"))
    if not roms:
        sys.exit(f"no ROM in {kind} {ident}")
    return roms[0]


def run(name, rom, frames, keep_save, probes):
    timeline = SMOKE / f"{name}.timeline"
    if not timeline.exists():
        sys.exit(f"no timeline {timeline.relative_to(ROOT)}")
    last, shots = timeline_info(timeline)
    out = OUT / name
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    if not keep_save:
        (SAVES / f"{rom.stem}.srm").unlink(missing_ok=True)
    env = dict(os.environ, SMOKE_TIMELINE=str(timeline), SMOKE_OUT=str(out),
               SMOKE_FRAMES=str(frames or last + 40))
    pokes = SMOKE / f"{name}.pokes"
    if pokes.exists():
        sym = rom.with_suffix(".sym")
        if not sym.exists():
            sys.exit(f"{name}: {pokes.name} needs {sym.name} next to the ROM")
        env["SMOKE_POKES"] = smoke_pokes(pokes, sym)
    if probes:
        env["SMOKE_PROBES"] = probes
    r = subprocess.run([MESEN, "--testRunner", "--doNotSaveSettings", "--noAudio",
                        "--debug.scriptWindow.allowIoOsAccess=true",
                        str(ROOT / "tools/mesen_smoke.lua"), str(rom)],
                       env=env, capture_output=True, text=True)
    noise = re.compile(r"Uninitialized memory read|^frame \d+ poke ")
    printed = [line for line in (r.stdout + r.stderr).splitlines()
               if line.strip() and not noise.search(line)]
    pngs = sorted(out.glob("frame-*.png"))
    if pngs:
        contact_sheet.sheet(OUT / f"{name}.png", pngs)
    return len(pngs), len([s for s in shots if s < (frames or last + 40)]), printed, pngs


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("names", nargs="*")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--rom")
    parser.add_argument("--release")
    parser.add_argument("--run")
    parser.add_argument("--frames", type=int)
    parser.add_argument("--keep-save", action="store_true")
    parser.add_argument("--probes")
    parser.add_argument("--no-build", action="store_true")
    args = parser.parse_args()

    names = sorted(p.stem for p in SMOKE.glob("*.timeline"))
    if args.list:
        for n in names:
            last, shots = timeline_info(SMOKE / f"{n}.timeline")
            extra = " +pokes" if (SMOKE / f"{n}.pokes").exists() else ""
            print(f"{n:16} {last:5} frames {len(shots):3} shots{extra}")
        return
    if args.all:
        args.names = names
    if not args.names:
        parser.error("name a timeline, or --all (--list shows them)")

    if args.rom:
        rom = Path(args.rom).resolve()
    elif args.release:
        rom = download("release", args.release)
    elif args.run:
        rom = download("run", args.run)
    elif args.no_build:
        rom = sorted((ROOT / "build/debug").glob("picross-*.sfc"))[-1]
    else:
        rom = build_debug()
    print(f"ROM: {rom.relative_to(ROOT) if rom.is_relative_to(ROOT) else rom}")

    failed, lasts = [], []
    for name in args.names:
        got, want, printed, pngs = run(name, rom, args.frames, args.keep_save, args.probes)
        ok = got >= want
        mark = "ok  " if ok else "FAIL"
        print(f"{mark} {name:16} {got}/{want} shots -> build/smoke/{name}.png")
        for line in printed[-8:]:
            print(f"       {line}")
        if not ok:
            failed.append(name)
        if pngs:
            lasts.append(str(pngs[-1]))
    if len(lasts) > 1:
        contact_sheet.sheet(OUT / "all.png", lasts, cols=4)
        print("last frames: build/smoke/all.png")
    if failed:
        sys.exit(f"failed: {', '.join(failed)}")


if __name__ == "__main__":
    main()
