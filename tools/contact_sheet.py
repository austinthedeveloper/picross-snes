#!/usr/bin/env python3
"""Tiles smoke screenshots into one PNG to review a run at a glance.

  python3 tools/contact_sheet.py OUT.png [--cols N] [--scale S] IN.png ...

Pure Python (Pillow isn't installed): decodes 8-bit RGB/RGBA PNGs, the
kind MesenCE writes, and labels nothing; files go left to right, top to
bottom, in the order given.
"""
import argparse
import struct
import zlib


def read_png(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos, idat, width, height, ctype = 8, b"", 0, 0, 0
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, ctype = struct.unpack(">IIBB", body[:10])
            assert depth == 8 and ctype in (2, 6), f"{path}: unsupported PNG"
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    bpp = 4 if ctype == 6 else 3
    raw = zlib.decompress(idat)
    stride = width * bpp
    rows, prev, i = [], bytearray(stride), 0
    for _ in range(height):
        ftype = raw[i]
        line = bytearray(raw[i + 1:i + 1 + stride])
        i += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 255
            elif ftype == 2:
                line[x] = (line[x] + b) & 255
            elif ftype == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else b if pb <= pc else c
                line[x] = (line[x] + pred) & 255
        rows.append([tuple(line[x * bpp:x * bpp + 3]) for x in range(width)])
        prev = line
    return width, height, rows


def write_png(path, width, height, rows):
    raw = b"".join(b"\0" + bytes(v for px in row for v in px) for row in rows)

    def chunk(kind, body):
        return (struct.pack(">I", len(body)) + kind + body +
                struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out")
    parser.add_argument("inputs", nargs="+")
    parser.add_argument("--cols", type=int, default=3)
    parser.add_argument("--scale", type=int, default=1)
    args = parser.parse_args()
    images = [read_png(p) for p in args.inputs]
    w = max(i[0] for i in images) * args.scale
    h = max(i[1] for i in images) * args.scale
    gap = 4
    cols = min(args.cols, len(images))
    nrows = (len(images) + cols - 1) // cols
    W, H = cols * w + (cols + 1) * gap, nrows * h + (nrows + 1) * gap
    canvas = [[(60, 60, 70)] * W for _ in range(H)]
    for n, (iw, ih, rows) in enumerate(images):
        ox = gap + (n % cols) * (w + gap)
        oy = gap + (n // cols) * (h + gap)
        for y in range(ih * args.scale):
            src = rows[y // args.scale]
            dst = canvas[oy + y]
            for x in range(iw * args.scale):
                dst[ox + x] = src[x // args.scale]
    write_png(args.out, W, H, canvas)


if __name__ == "__main__":
    main()
