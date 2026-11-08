#!/usr/bin/env python3
"""Starts a release (ADR-009): the version bump and its notes.

  python3 tools/release.py 0.3.0            branch, bump, draft the notes
  python3 tools/release.py 0.3.0 --check    is everything ready to tag?

The first form, from an up-to-date clean main: creates the branch
feature/cld-release-0.3.0, sets VERSION_NAME in the Makefile, and drafts
docs/releases/v0.3.0.md from the merged PRs since the last tag (one line
each, to rewrite for players). It commits nothing and pushes nothing:
edit the notes, commit, open the PR.

--check, after that PR merged: main has the version and the notes, the
tag doesn't exist yet, and CI on main passed. It prints the two commands
that tag and push; pushing the tag publishes the release.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = "austinthedeveloper/picross-snes"


def sh(*args, check=True):
    r = subprocess.run(args, cwd=ROOT, capture_output=True, text=True)
    if check and r.returncode:
        sys.exit(f"{' '.join(args)}: {r.stderr.strip()}")
    return r.stdout.strip()


def version_in_makefile():
    m = re.search(r"^VERSION_NAME := (\S+)$", (ROOT / "Makefile").read_text(), re.M)
    return m.group(1)


def last_tag():
    return sh("git", "describe", "--tags", "--abbrev=0", "origin/main", check=False) or None


def merged_since(tag):
    since = ""
    if tag:
        since = sh("git", "log", "-1", "--format=%cI", tag)
    prs = json.loads(sh("gh", "pr", "list", "-R", REPO, "--state", "merged", "--base", "main",
                        "--limit", "100", "--json", "number,title,mergedAt"))
    return [p for p in prs if not since or p["mergedAt"] > since]


def start(version):
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        sys.exit("version looks like 0.3.0")
    if sh("git", "status", "--porcelain"):
        sys.exit("the working tree isn't clean")
    sh("git", "fetch", "-q", "origin")
    sh("git", "checkout", "-q", "main")
    sh("git", "merge", "-q", "--ff-only", "origin/main")
    branch = f"feature/cld-release-{version}"
    sh("git", "checkout", "-q", "-b", branch)
    mk = ROOT / "Makefile"
    mk.write_text(re.sub(r"^VERSION_NAME := \S+$", f"VERSION_NAME := {version}",
                         mk.read_text(), count=1, flags=re.M))
    tag = last_tag()
    lines = [f"Everything since {tag}:" if tag else "The first release:", ""]
    for p in sorted(merged_since(tag), key=lambda p: p["number"]):
        lines.append(f"- {p['title']} (#{p['number']})")
    lines += ["", "<!-- Rewrite the list for players: what's new, what's fixed. -->", ""]
    notes = ROOT / "docs" / "releases" / f"v{version}.md"
    notes.write_text("\n".join(lines), encoding="utf-8")
    print(f"on {branch}: VERSION_NAME {version}, drafted {notes.relative_to(ROOT)}")
    print("Edit the notes, then commit and open the PR.")


def check(version):
    sh("git", "fetch", "-q", "--tags", "origin")
    problems = []
    makefile = sh("git", "show", "origin/main:Makefile")
    if f"VERSION_NAME := {version}" not in makefile:
        problems.append(f"main's Makefile isn't at {version}")
    notes = subprocess.run(["git", "cat-file", "-e", f"origin/main:docs/releases/v{version}.md"],
                           cwd=ROOT, capture_output=True)
    if notes.returncode:
        problems.append(f"main has no docs/releases/v{version}.md")
    if sh("git", "tag", "-l", f"v{version}"):
        problems.append(f"v{version} is already tagged")
    runs = json.loads(sh("gh", "run", "list", "-R", REPO, "--branch", "main", "--workflow", "ci.yml",
                         "--limit", "1", "--json", "conclusion,headSha"))
    head = sh("git", "rev-parse", "origin/main")
    if not runs or runs[0]["headSha"] != head or runs[0]["conclusion"] != "success":
        problems.append("CI hasn't passed on main's latest commit yet")
    if problems:
        sys.exit("not ready:\n  " + "\n  ".join(problems))
    print(f"ready. To publish v{version}:")
    print(f'  git tag -a v{version} origin/main -m "Picross \'94 v{version}"')
    print(f"  git push origin v{version}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("version")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    (check if args.check else start)(args.version)


if __name__ == "__main__":
    main()
