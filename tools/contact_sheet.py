#!/usr/bin/env python3
"""Tiles smoke screenshots into one PNG to review a run at a glance.

  python3 tools/contact_sheet.py OUT.png [--cols N] [--scale S] IN.png ...

Pure Python (Pillow isn't installed; tools/png.py does the decoding).
Alpha is dropped and nothing is labelled; files go left to right, top to
bottom, in the order given.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from png import read_png, write_png  # noqa: E402


def sheet(out, inputs, cols=3, scale=1):
    """Tiles the PNGs in inputs into out (tools/smoke.py calls this)."""
    images = [read_png(p) for p in inputs]
    w = max(i[0] for i in images) * scale
    h = max(i[1] for i in images) * scale
    gap = 4
    cols = min(cols, len(images))
    nrows = (len(images) + cols - 1) // cols
    W, H = cols * w + (cols + 1) * gap, nrows * h + (nrows + 1) * gap
    canvas = [[(60, 60, 70)] * W for _ in range(H)]
    for n, (iw, ih, rows) in enumerate(images):
        ox = gap + (n % cols) * (w + gap)
        oy = gap + (n // cols) * (h + gap)
        for y in range(ih * scale):
            src = rows[y // scale]
            dst = canvas[oy + y]
            for x in range(iw * scale):
                dst[ox + x] = src[x // scale]
    write_png(str(out), W, H, canvas)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out")
    parser.add_argument("inputs", nargs="+")
    parser.add_argument("--cols", type=int, default=3)
    parser.add_argument("--scale", type=int, default=1)
    args = parser.parse_args()
    sheet(args.out, args.inputs, args.cols, args.scale)


if __name__ == "__main__":
    main()
