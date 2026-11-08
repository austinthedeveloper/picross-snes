"""Sound effects (ADR-006): synthesized samples in an Impulse Tracker file.

PVSnesLib's smconv turns the IT file into a SNESMOD soundbank; each sample
is one effect (spcLoadEffect / spcEffect). Everything is synthesized here
with the standard library, deterministically (fixed seeds), so the build
needs no recorded audio. The IT writer and the helpers come from Pickleball
'94's tools/assetgen/sfx.py.
"""
import math
import random
import struct

RATE = 16000                    # Hz; played back with SNESMOD pitch 4
# (name, synth function): the order is the effect index (sfx.h).
EFFECTS = (
    ("MOVE", "move"),           # the cursor steps: a soft tick
    ("FILL", "fill"),           # a cell filled: a pencil tap
    ("MARK", "mark"),           # an X: a short scratch
    ("ERASE", "erase"),         # a cell cleared: a duller tap
    ("LINE", "line"),           # a row or column matches its clues
    ("MISTAKE", "mistake"),     # hearts mode: a wrong fill
    ("HINT", "hint"),           # a hint reveals cells
    ("SOLVE", "solve"),         # the picture is complete
    ("NEWS", "news"),           # a rank, badge or achievement
    ("BLIP", "blip"),           # menu cursor
    ("CONFIRM", "confirm"),     # menu choice
    ("BACK", "back"),           # menu back
)
PEAK = 0.85


def samples(seconds):
    return range(int(seconds * RATE))


def decay(t, tau):
    return math.exp(-t / tau)


def tone(freqs, seconds, tau, harmonics=(1.0, 0.0, 0.25)):
    out = []
    span = len(samples(seconds))
    for n in range(span):
        t = n / RATE
        step = min(len(freqs) - 1, n * len(freqs) // span)
        freq = freqs[step]
        local = t - step * seconds / len(freqs)
        value = sum(a * math.sin(2 * math.pi * freq * (i + 1) * t)
                    for i, a in enumerate(harmonics) if a)
        out.append(value * decay(local, tau) * min(1.0, t / 0.002))
    return out


def move():
    return tone([2200], 0.02, 0.006, (1.0,))


def fill():
    rng = random.Random(1)
    out = []
    for n in samples(0.06):
        t = n / RATE
        body = math.sin(2 * math.pi * 620 * t) + 0.5 * math.sin(2 * math.pi * 1240 * t)
        click = (rng.random() * 2 - 1) * decay(t, 0.002)
        out.append(body * decay(t, 0.014) + 0.6 * click)
    return out


def mark():
    rng = random.Random(2)
    out = []
    previous = 0.0
    for n in samples(0.07):
        t = n / RATE
        noise = rng.random() * 2 - 1
        previous = 0.5 * previous + 0.5 * noise      # a little low-passed
        two_strokes = decay(t, 0.012) + 0.8 * decay(max(0.0, t - 0.035), 0.012) * (t > 0.035)
        out.append(previous * two_strokes)
    return out


def erase():
    rng = random.Random(3)
    out = []
    for n in samples(0.06):
        t = n / RATE
        body = math.sin(2 * math.pi * 330 * t)
        rub = (rng.random() * 2 - 1) * decay(t, 0.02) * 0.3
        out.append(body * decay(t, 0.018) + rub)
    return out


def line():
    return tone([1568, 2093], 0.16, 0.06)


def mistake():
    out = []
    for n in samples(0.28):
        t = n / RATE
        square = 1.0 if math.sin(2 * math.pi * 116 * t) > 0 else -1.0
        wobble = 1.0 if math.sin(2 * math.pi * 123 * t) > 0 else -1.0
        env = min(1.0, t / 0.005) * min(1.0, (0.28 - t) / 0.06)
        out.append(0.5 * (square + wobble) * env)
    return out


def hint():
    return tone([2637, 2093, 1760, 2349, 3136], 0.3, 0.05)


def solve():
    # C major arpeggio up, with the top note held.
    notes = [523, 659, 784, 1047]
    out = []
    step = 0.1
    length = step * 3 + 0.5
    for n in samples(length):
        t = n / RATE
        value = 0.0
        for i, f in enumerate(notes):
            start = i * step
            if t >= start:
                local = t - start
                tau = 0.35 if i == len(notes) - 1 else 0.12
                value += (math.sin(2 * math.pi * f * local) +
                          0.3 * math.sin(2 * math.pi * 2 * f * local)) * decay(local, tau)
        out.append(value * min(1.0, (length - t) / 0.05))
    return out


def news():
    return tone([1047, 1319, 1568, 2093], 0.36, 0.08)


def blip():
    return tone([1320], 0.04, 0.015)


def confirm():
    return tone([880, 1320], 0.14, 0.05)


def back():
    return tone([660, 440], 0.1, 0.04)


def pcm16(values):
    """Normalized to PEAK, as signed 16-bit."""
    top = max(abs(v) for v in values) or 1.0
    return [max(-32767, min(32767, round(v / top * PEAK * 32767)))
            for v in values]


def pack_pattern(rows, events):
    """IT pattern data. events: {row: [(channel, note, instrument,
    volume)]}, channel 0-based, instrument 1-based, volume 0-64."""
    data = bytearray()
    for row in range(rows):
        for channel, note, instrument, volume in events.get(row, ()):
            data.append((channel + 1) | 0x80)
            data.append(0x07)
            data += bytes([note, instrument, volume])
        data.append(0)
    return struct.pack("<HH4x", len(data), rows) + bytes(data)


def it_module(title, samples_, patterns, orders, speed=6, tempo=125):
    """A minimal Impulse Tracker module: one 16-bit sample and one
    instrument per sample, and the given patterns and order list."""
    count = len(samples_)
    order_bytes = bytes(list(orders) + [255])
    header = bytearray(0xC0)
    header[0:4] = b"IMPM"
    name = title.encode()[:25]
    header[4:4 + len(name)] = name
    struct.pack_into("<HHHH", header, 0x20, len(order_bytes), count, count,
                     len(patterns))
    struct.pack_into("<HHHH", header, 0x28, 0x0214, 0x0214, 0x0D, 0)
    header[0x30:0x36] = bytes([128, 48, speed, tempo, 128, 0])
    for ch in range(64):
        header[0x40 + ch] = 32
        header[0x80 + ch] = 64
    offsets_size = 4 * (count + count + len(patterns))
    cursor = 0xC0 + len(order_bytes) + offsets_size

    instruments = []
    for i in range(count):
        ins = bytearray(554)
        ins[0:4] = b"IMPI"
        ins[0x18] = 128
        ins[0x19] = 32 | 128
        label = samples_[i][0].encode()[:25]
        ins[0x20:0x20 + len(label)] = label
        for note in range(120):
            ins[0x40 + note * 2] = note
            ins[0x40 + note * 2 + 1] = i + 1
        instruments.append(bytes(ins))
    ins_offsets = []
    for ins in instruments:
        ins_offsets.append(cursor)
        cursor += len(ins)

    sample_headers_at = cursor
    cursor += 0x50 * count
    packed = [pack_pattern(rows, events) for rows, events in patterns]
    pattern_offsets = []
    for pat in packed:
        pattern_offsets.append(cursor)
        cursor += len(pat)

    sample_headers = []
    data = []
    for name, pcm, rate, loop in samples_:
        smp = bytearray(0x50)
        smp[0:4] = b"IMPS"
        smp[0x11] = 64
        smp[0x12] = 0x03 | (0x10 if loop else 0)
        smp[0x13] = 64
        label = name.encode()[:25]
        smp[0x14:0x14 + len(label)] = label
        smp[0x2E] = 0x01
        smp[0x2F] = 32
        struct.pack_into("<IIII", smp, 0x30, len(pcm), 0,
                         len(pcm) if loop else 0, rate)
        struct.pack_into("<I", smp, 0x48, cursor)
        raw = struct.pack(f"<{len(pcm)}h", *pcm)
        data.append(raw)
        cursor += len(raw)
        sample_headers.append(bytes(smp))

    smp_offsets = [sample_headers_at + 0x50 * i for i in range(count)]
    out = bytearray(header)
    out += order_bytes
    out += struct.pack(f"<{count}I", *ins_offsets)
    out += struct.pack(f"<{count}I", *smp_offsets)
    out += struct.pack(f"<{len(patterns)}I", *pattern_offsets)
    for ins in instruments:
        out += ins
    for smp in sample_headers:
        out += smp
    for pat in packed:
        out += pat
    for raw in data:
        out += raw
    return bytes(out)


def sfx_assets():
    """sfx.it for smconv, and sfx.h with the effect indices."""
    effects = [(name, pcm16(globals()[fn]())) for name, fn in EFFECTS]
    lines = ["/* Generated by tools/gen_assets.py (tools/sfx.py). Do not edit. */",
             "#ifndef SFX_H", "#define SFX_H", "",
             "/* Effect indices in the soundbank (ADR-006), 16 kHz samples: "
             "play at pitch 4. */"]
    for i, (name, _) in enumerate(EFFECTS):
        lines.append(f"#define SFX_{name} {i}")
    lines += [f"#define SFX_COUNT {len(EFFECTS)}", "#define SFX_PITCH 4", "",
              "#endif", ""]
    module = it_module("PICROSS SFX",
                       [(name, pcm, RATE, False) for name, pcm in effects],
                       [(64, {})], [0])
    seconds = sum(len(pcm) for _, pcm in effects) / RATE
    return module, "\n".join(lines), seconds
