"""Pure-Python PNG reading and writing (Pillow isn't installed).

  read_png_rgba(path) -> (width, height, rows of (r, g, b, a) tuples)
  read_png(path)      -> (width, height, rows of (r, g, b) tuples)
  write_png(path, width, height, rows)   rows of (r, g, b) tuples

The reader takes every non-interlaced PNG: greyscale, RGB, palette,
grey+alpha and RGBA, at any legal bit depth (1-16). Values come back as
8-bit channels: 16-bit samples keep their high byte, low depths are scaled
up, and a tRNS chunk becomes alpha. Interlaced (Adam7) files are rejected.
"""
import struct
import zlib

SIGNATURE = b"\x89PNG\r\n\x1a\n"
CHANNELS = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}
DEPTHS = {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8), 4: (8, 16), 6: (8, 16)}


def _chunks(data, path):
    if data[:8] != SIGNATURE:
        raise ValueError(f"{path}: not a PNG")
    pos = 8
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        yield kind, data[pos + 8:pos + 8 + length]
        pos += 12 + length


def _unfilter(raw, height, stride, bpp):
    """Undoes the per-row filters; yields each row's bytes."""
    prev, i = bytearray(stride), 0
    for _ in range(height):
        ftype = raw[i]
        line = bytearray(raw[i + 1:i + 1 + stride])
        i += 1 + stride
        if ftype == 1:
            for x in range(bpp, stride):
                line[x] = (line[x] + line[x - bpp]) & 255
        elif ftype == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif ftype == 3:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 255
        elif ftype == 4:
            for x in range(stride):
                if x >= bpp:
                    a, c = line[x - bpp], prev[x - bpp]
                else:
                    a = c = 0
                b = prev[x]
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else b if pb <= pc else c
                line[x] = (line[x] + pred) & 255
        elif ftype != 0:
            raise ValueError(f"bad PNG filter type {ftype}")
        yield line
        prev = line


def _samples(line, width, channels, depth):
    """One row's samples (channels * width of them) at their native depth."""
    if depth == 8:
        return line
    if depth == 16:
        return line[0::2]  # high bytes; scaled to 8 bits already
    count, per_byte, mask = width * channels, 8 // depth, (1 << depth) - 1
    out = []
    for byte in line:
        for k in range(per_byte):
            out.append((byte >> (8 - depth * (k + 1))) & mask)
    return out[:count]


def read_png_rgba(path):
    data = open(path, "rb").read()
    width = height = depth = ctype = 0
    idat, palette, trns = [], None, None
    for kind, body in _chunks(data, path):
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body[:13])
            if ctype not in CHANNELS or depth not in DEPTHS[ctype]:
                raise ValueError(f"{path}: unsupported PNG (type {ctype}, depth {depth})")
            if interlace:
                raise ValueError(f"{path}: interlaced PNGs are not supported; re-save it non-interlaced")
        elif kind == b"PLTE":
            palette = [tuple(body[i:i + 3]) for i in range(0, len(body) - 2, 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat.append(body)
    channels = CHANNELS[ctype]
    bits = channels * depth
    stride = (width * bits + 7) // 8
    bpp = max(1, bits // 8)
    raw = zlib.decompress(b"".join(idat))

    if ctype == 3:
        if palette is None:
            raise ValueError(f"{path}: palette PNG without PLTE")
        alpha = list(trns or b"") + [255] * (256 - len(trns or b""))
        lut = [palette[i] + (alpha[i],) if i < len(palette) else (0, 0, 0, 255)
               for i in range(256)]
    scale = 255 // ((1 << depth) - 1) if depth < 8 else 1
    key = None
    if trns and ctype in (0, 2):
        vals = struct.unpack(">" + "H" * (len(trns) // 2), trns)
        key = tuple((v >> 8 if depth == 16 else v * scale) for v in vals)

    rows = []
    for line in _unfilter(raw, height, stride, bpp):
        s = _samples(line, width, channels, depth)
        if ctype == 6:
            row = [tuple(s[i:i + 4]) for i in range(0, 4 * width, 4)]
        elif ctype == 2:
            row = [tuple(s[i:i + 3]) + (255,) for i in range(0, 3 * width, 3)]
            if key:
                row = [p[:3] + (0,) if p[:3] == key else p for p in row]
        elif ctype == 3:
            row = [lut[v] for v in s]
        elif ctype == 4:
            row = [(s[i], s[i], s[i], s[i + 1]) for i in range(0, 2 * width, 2)]
        else:
            row = [(v * scale,) * 3 + (0 if key and v * scale == key[0] else 255,) for v in s]
        rows.append(row)
    return width, height, rows


def read_png(path):
    width, height, rows = read_png_rgba(path)
    return width, height, [[p[:3] for p in row] for row in rows]


def write_png(path, width, height, rows):
    raw = b"".join(b"\0" + bytes(v for px in row for v in px) for row in rows)

    def chunk(kind, body):
        return (struct.pack(">I", len(body)) + kind + body +
                struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(SIGNATURE)
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))
