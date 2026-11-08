#!/usr/bin/env python3
"""Lists functions declared in include/*.h that nothing calls.

  python3 tools/find_unused.py [--strict]

A function counts as used when its name appears in src/*.c, src/asm/*.asm
or test/*.c anywhere other than its own definition line. Functions only the
host tests call are listed separately (they may be fine: test helpers, or a
sign the game lost a caller). --strict exits 1 if anything is unused, for
make check.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECL = re.compile(r"^[A-Za-z_][\w \*]*?\b([a-z_][a-z0-9_]*)\s*\([^;{]*\)\s*;", re.M)
WORD = r"\b{}\b"


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    declared = {}
    for h in sorted((ROOT / "include").glob("*.h")):
        for m in DECL.finditer(strip_comments(h.read_text(encoding="utf-8"))):
            declared.setdefault(m.group(1), h.name)

    game = {p: strip_comments(p.read_text(encoding="utf-8"))
            for p in list((ROOT / "src").glob("*.c")) + list((ROOT / "src/asm").glob("*.asm"))}
    tests = {p: strip_comments(p.read_text(encoding="utf-8")) for p in (ROOT / "test").glob("*.c")}

    def uses(name, files):
        pat = re.compile(WORD.format(re.escape(name)))
        defn = re.compile(rf"^[A-Za-z_][\w \*]*\b{re.escape(name)}\s*\([^;]*$", re.M)
        n = 0
        for text in files.values():
            n += len(pat.findall(text)) - len(defn.findall(text))
        return n

    unused, test_only = [], []
    for name, header in sorted(declared.items()):
        if uses(name, game) > 0:
            continue
        (test_only if uses(name, tests) > 0 else unused).append(f"{name} ({header})")
    if unused:
        print("Never called:\n  " + "\n  ".join(unused))
    if test_only and not args.strict:
        print("Only the host tests call:\n  " + "\n  ".join(test_only))
    if not unused:
        print("unused functions: none" + (f" ({len(test_only)} test-only)" if args.strict else ""))
    sys.exit(1 if args.strict and unused else 0)


if __name__ == "__main__":
    main()
