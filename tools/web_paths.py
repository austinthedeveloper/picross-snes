"""Finds a file or folder in the web game's checkout, whichever layout it has.

The web repo was one app with everything under src/ and public/. It is now a
monorepo: the 2D puzzles in packages/grid, the story in packages/story, art
and shared code in packages/shared, the rest in apps/picross. packages/block
(the 3D game) is left out: it has its own puzzles/index.ts, pack/ and game.ts.
"""

from pathlib import Path

# Searched in order; the first that has the path wins.
ROOTS = [
    "",                     # the old single-app layout
    "packages/grid",
    "packages/story",
    "packages/shared",
    "apps/picross",
]


def find(web, rel, top="src"):
    """`web`'s `top`/`rel` (top is "src" or "public"), or the old-layout path if none exists."""
    web = Path(web)
    for root in ROOTS:
        path = web / root / top / rel
        if path.exists():
            return path
    return web / top / rel


def src(web, rel):
    return find(web, rel, "src")


def public(web, rel):
    return find(web, rel, "public")
