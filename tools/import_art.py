#!/usr/bin/env python3
"""Converts the web game's PNG art into small SNES-ready indexed images.

  python3 tools/import_art.py [--from PATH_TO_WEB_REPO] [--check] [--preview]

Reads public/art/<group>/*.png from the web repo (default ../../picross, the
sibling checkout) and writes data/art/<group>/<id>.txt, committed, so the ROM
build never needs the web repo. Each image is:

  1. scaled to fit its size in SIZES (aspect kept, centred), by area
     averaging on premultiplied alpha. Dark pixels weigh more in the colour
     average (DARK_WEIGHT), so one-pixel outlines and eyes survive the
     shrink instead of fading into the fill around them;
  2. cut to two alpha levels: at least half covered is opaque, the rest is
     colour 0 (transparent);
  3. quantized to at most 15 colours in SNES colour space (BGR555): median
     cut, then a few k-means passes, all deterministic, so a re-run writes
     byte-identical files.

File format (data/art/<group>/<id>.txt):

  size W H
  #RRGGBB          one line per palette colour, index 1 first (at most 15);
  ...              8-bit values expanded from 5 bits: (v5 << 3) | (v5 >> 2)
  <blank line>
  H rows of W characters: "." transparent, 1-9 and a-f palette index 1-15

--check writes nothing and exits 1 if any file would change, or one is
missing or extra. Without it, .txt files whose PNG disappeared are deleted.
--preview also writes build/art-preview.png: every image at 4x with its
palette underneath, for judging the conversion by eye.
"""
import argparse
import fnmatch
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from png import read_png_rgba, write_png  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data" / "art"
PREVIEW = ROOT / "build" / "art-preview.png"

# (group, file name pattern, width, height): the first match wins. A PNG no
# rule matches is an error, so new art gets a size on purpose.
SIZES = [
    ("cast", "*", 64, 64),
    ("enemies", "*", 48, 48),
    ("gadgets", "*", 16, 16),
    ("icons", "glyph-*", 8, 8),
    ("icons", "boss", 16, 16),
    ("icons", "fight", 16, 16),
    ("icons", "dig", 16, 16),
    ("icons", "rest", 16, 16),
]
GROUPS = sorted({group for group, _, _, _ in SIZES})

MAX_COLOURS = 15
DARK_WEIGHT = 1.5  # a black pixel weighs 1 + this in the colour average; white 1
# (0 is a plain average: faces go pale; 6 and up muddies them with hatching)
KMEANS_PASSES = 8
INDEX_CHARS = ".123456789abcdef"
# Squared-distance weights per channel (R, G, B): rough perceived difference.
CHANNEL_WEIGHTS = (3, 4, 2)


def size_for(group, name):
    for g, pattern, w, h in SIZES:
        if g == group and fnmatch.fnmatchcase(name, pattern):
            return w, h
    return None


# ---- scaling ---------------------------------------------------------------

def _taps(n, m, offset):
    """Area weights mapping n source samples onto m outputs, numbered from
    offset: [(output index, [(source index, weight), ...]), ...]."""
    scale = n / m
    out = []
    for i in range(m):
        lo, hi = i * scale, (i + 1) * scale
        taps, j = [], int(lo)
        while j < hi and j < n:
            w = min(hi, j + 1) - max(lo, j)
            if w > 1e-9:
                taps.append((j, w / scale))
            j += 1
        out.append((i + offset, taps))
    return out


def downscale(width, height, rows, out_w, out_h):
    """Area-averaged RGBA (floats, straight alpha) of out_h rows of out_w."""
    fit = min(out_w / width, out_h / height)
    m_w = max(1, round(width * fit))
    m_h = max(1, round(height * fit))
    xtaps = _taps(width, m_w, (out_w - m_w) // 2)
    ytaps = _taps(height, m_h, (out_h - m_h) // 2)

    # Per source pixel: alpha, and colour weighted by alpha * darkness bias.
    src = []
    for row in rows:
        line = []
        for r, g, b, a in row:
            if a == 0:
                line.append((0.0, 0.0, 0.0, 0.0, 0.0))
                continue
            af = a / 255
            dark = 1 - (0.299 * r + 0.587 * g + 0.114 * b) / 255
            w = af * (1 + DARK_WEIGHT * dark * dark)
            line.append((af, w * r, w * g, w * b, w))
        src.append(line)

    zero = (0.0, 0.0, 0.0, 0.0, 0.0)
    horiz = []
    for line in src:
        out = [zero] * out_w
        for x, taps in xtaps:
            acc = [0.0] * 5
            for j, w in taps:
                px = line[j]
                for k in range(5):
                    acc[k] += px[k] * w
            out[x] = acc
        horiz.append(out)

    result = [[(0.0, 0.0, 0.0, 0.0)] * out_w for _ in range(out_h)]
    for y, taps in ytaps:
        for x in range(out_w):
            acc = [0.0] * 5
            for j, w in taps:
                px = horiz[j][x]
                for k in range(5):
                    acc[k] += px[k] * w
            a, wr, wg, wb, wt = acc
            if wt > 0:
                result[y][x] = (wr / wt, wg / wt, wb / wt, a)
    return result


# ---- quantizing ------------------------------------------------------------

def to5(v):
    return max(0, min(31, int(v * 31 / 255 + 0.5)))


def expand5(v5):
    return (v5 << 3) | (v5 >> 2)


def _dist(p, q):
    return sum(w * (a - b) * (a - b) for w, a, b in zip(CHANNEL_WEIGHTS, p, q))


def _nearest(colour, palette):
    best, best_d = 0, None
    for i, p in enumerate(palette):
        d = _dist(colour, p)
        if best_d is None or d < best_d:
            best, best_d = i, d
    return best


def _mean(box):
    total = sum(n for _, n in box)
    return tuple(sum(c[k] * n for c, n in box) / total for k in range(3))


def median_cut(counts, k):
    """counts: [((r5, g5, b5), n), ...] sorted. Up to k centre colours."""
    boxes = [counts]
    while len(boxes) < k:
        best, best_score, best_axis = None, 0, 0
        for i, box in enumerate(boxes):
            if len(box) < 2:
                continue
            total = sum(n for _, n in box)
            for axis in range(3):
                vals = [c[axis] for c, _ in box]
                spread = (max(vals) - min(vals)) * CHANNEL_WEIGHTS[axis] ** 0.5
                score = spread * total ** 0.5
                if score > best_score:
                    best, best_score, best_axis = i, score, axis
        if best is None:
            break
        box = sorted(boxes.pop(best), key=lambda cn: (cn[0][best_axis], cn[0]))
        half, run, cut = sum(n for _, n in box) / 2, 0, 1
        for cut in range(1, len(box)):
            run += box[cut - 1][1]
            if run >= half:
                break
        boxes[best:best] = [box[:cut], box[cut:]]
    return [_mean(box) for box in boxes]


def kmeans(counts, centres, passes):
    for _ in range(passes):
        groups = [[] for _ in centres]
        for c, n in counts:
            groups[_nearest(c, centres)].append((c, n))
        new = [_mean(g) for g in groups if g]
        if new == centres:
            break
        centres = new
    return centres


def quantize(pixels):
    """pixels: rows of (r, g, b, a) floats. Returns (palette of 5-bit
    triples, rows of indexes 0-15)."""
    five = [[(to5(r), to5(g), to5(b)) if a >= 0.5 else None for r, g, b, a in row]
            for row in pixels]
    tally = {}
    for row in five:
        for c in row:
            if c is not None:
                tally[c] = tally.get(c, 0) + 1
    counts = sorted(tally.items())
    if len(counts) <= MAX_COLOURS:
        palette = [c for c, _ in counts]
    else:
        centres = kmeans(counts, median_cut(counts, MAX_COLOURS), KMEANS_PASSES)
        palette = sorted({tuple(int(v + 0.5) for v in c) for c in centres})
        # Drop palette entries no pixel ends up using.
        used = {_nearest(c, palette) for c, _ in counts}
        palette = [p for i, p in enumerate(palette) if i in used]
    # Darkest first, so files read in a stable, sensible order.
    palette.sort(key=lambda c: (CHANNEL_WEIGHTS[0] * c[0] + CHANNEL_WEIGHTS[1] * c[1]
                                + CHANNEL_WEIGHTS[2] * c[2], c))
    lookup = {c: _nearest(c, palette) + 1 for c in tally}
    return palette, [[0 if c is None else lookup[c] for c in row] for row in five]


# ---- files -----------------------------------------------------------------

def render(width, height, palette, indexes):
    lines = [f"size {width} {height}"]
    for r, g, b in palette:
        lines.append("#%02X%02X%02X" % (expand5(r), expand5(g), expand5(b)))
    lines.append("")
    lines += ["".join(INDEX_CHARS[i] for i in row) for row in indexes]
    return "\n".join(lines) + "\n"


def convert(png_path, out_w, out_h):
    width, height, rows = read_png_rgba(png_path)
    palette, indexes = quantize(downscale(width, height, rows, out_w, out_h))
    return palette, indexes


def collect(art_dir):
    """[(group, id, png path, w, h)], sorted; errors for unsized PNGs."""
    jobs, errors = [], []
    for group in GROUPS:
        for png in sorted((art_dir / group).glob("*.png")):
            size = size_for(group, png.stem)
            if size is None:
                errors.append(f"{group}/{png.name}: no size rule; add one to SIZES")
                continue
            jobs.append((group, png.stem, png, *size))
    return jobs, errors


def write_preview(results):
    """results: [(group, id, w, h, palette, indexes)]. Tiles at 4x."""
    scale, swatch, gap, cols = 4, 12, 8, 8
    bg, checker = (70, 92, 108), (82, 106, 122)
    cell_w = max(max(w for _, _, w, _, _, _ in results) * scale, MAX_COLOURS * swatch)
    cell_h = max(h for _, _, _, h, _, _ in results) * scale + 4 + swatch
    layout, row, col, last_group = [], 0, 0, None
    for res in results:
        if last_group is not None and (res[0] != last_group or col == cols):
            row, col = row + 1, 0
        layout.append((res, row, col))
        last_group, col = res[0], col + 1
    nrows = row + 1
    W = cols * cell_w + (cols + 1) * gap
    H = nrows * cell_h + (nrows + 1) * gap
    canvas = [[bg] * W for _ in range(H)]
    for (group, _, w, h, palette, indexes), r, c in layout:
        ox, oy = gap + c * (cell_w + gap), gap + r * (cell_h + gap)
        colours = [None] + [tuple(expand5(v) for v in p) for p in palette]
        for y in range(h * scale):
            line = canvas[oy + y]
            src = indexes[y // scale]
            for x in range(w * scale):
                i = src[x // scale]
                if i:
                    line[ox + x] = colours[i]
                elif ((x // 4) + (y // 4)) & 1:
                    line[ox + x] = checker
        sy = oy + h * scale + 4
        for n, colour in enumerate(colours[1:]):
            for y in range(sy, sy + swatch - 2):
                for x in range(ox + n * swatch, ox + n * swatch + swatch - 2):
                    canvas[y][x] = colour
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    write_png(PREVIEW, W, H, canvas)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--from", dest="src", default=str(ROOT.parent.parent / "picross"))
    parser.add_argument("--check", action="store_true",
                        help="write nothing; exit 1 if any output would change")
    parser.add_argument("--preview", action="store_true",
                        help=f"also write {PREVIEW.relative_to(ROOT)}")
    args = parser.parse_args()

    art_dir = Path(args.src) / "public" / "art"
    if not art_dir.is_dir():
        sys.exit(f"import_art: no art at {art_dir}")
    jobs, errors = collect(art_dir)
    if errors:
        sys.exit("import_art: " + "\n  ".join(errors))

    results, stale, wanted = [], [], set()
    for group, name, png, w, h in jobs:
        palette, indexes = convert(png, w, h)
        results.append((group, name, w, h, palette, indexes))
        out = OUT / group / f"{name}.txt"
        wanted.add(out)
        text = render(w, h, palette, indexes)
        if not out.exists() or out.read_text() != text:
            stale.append(out)
            if not args.check:
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(text)
    extra = sorted(p for p in OUT.glob("*/*.txt") if p not in wanted) if OUT.is_dir() else []

    if args.check:
        for p in stale:
            print(f"import_art: out of date: {p.relative_to(ROOT)}")
        for p in extra:
            print(f"import_art: no source PNG: {p.relative_to(ROOT)}")
        if stale or extra:
            sys.exit("import_art: data/art is out of date; run python3 tools/import_art.py")
    else:
        for p in extra:
            p.unlink()
        print(f"import_art: {len(results)} images, {len(stale)} written, {len(extra)} deleted")
    if args.preview:
        write_preview(results)
        print(f"import_art: preview in {PREVIEW.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
