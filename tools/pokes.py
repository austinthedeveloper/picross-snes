#!/usr/bin/env python3
"""Turns a .pokes file into SMOKE_POKES for tools/mesen_smoke.lua.

  python3 tools/pokes.py test/smoke/NAME.pokes build/debug/ROM.sym

Each line: FRAME SYMBOL[+OFFSET] SIZE VALUE  ('#' starts a comment), and
"repeat N STRIDE" before a line pokes it N times, OFFSET + i*STRIDE. The
write happens once, at the end of FRAME (the game sees it on the next).
Symbols resolve through the ROM's .sym file, so pokes survive relinking.
"""
import sys


def main():
    pokes_path, sym_path = sys.argv[1], sys.argv[2]
    syms = {}
    for line in open(sym_path, encoding="utf-8"):
        parts = line.split()
        if len(parts) == 2 and not line.startswith(";"):
            syms[parts[1]] = int(parts[0], 16)
    out = []
    repeat = None
    for raw in open(pokes_path, encoding="utf-8"):
        line = raw.split("#")[0].split()
        if not line:
            continue
        if line[0] == "repeat":
            repeat = (int(line[1]), int(line[2]))
            continue
        frame, target, size, value = line
        name, _, off = target.partition("+")
        # Work RAM offset: bank $7E is 0-$FFFF, bank $7F is $10000 on.
        addr = (syms[name] & 0xFFFF) | (((syms[name] >> 16) & 1) << 16)
        base = addr + (int(off, 0) if off else 0)
        count, stride = repeat or (1, 0)
        for i in range(count):
            out.append(f"{frame},{frame},1,0,0,0,{base + i * stride},{size},{value}")
        repeat = None
    print(";".join(out))


if __name__ == "__main__":
    main()
