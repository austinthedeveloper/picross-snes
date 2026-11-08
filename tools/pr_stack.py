#!/usr/bin/env python3
"""Shows and merges stacked pull requests (a PR based on another PR).

  python3 tools/pr_stack.py                 show every open stack
  python3 tools/pr_stack.py merge TOP       merge TOP's stack, bottom first
          [--delete-branches] [--dry-run]
  python3 tools/pr_stack.py strays          merged PRs whose work never
                                            reached main

A stack is a chain of open PRs where each one's base is the branch of the
one below, down to one based on main. Merging goes bottom up: each PR is
retargeted to main before it merges, so deleting a merged branch can never
close the PR above it (GitHub closes PRs whose base branch disappears).
Each PR merges with a merge commit, like every PR here, and only once
GitHub says it's mergeable. Branches are deleted only with
--delete-branches, and only after the whole stack merged.

Only run merge when asked to merge: it changes main.
"""
import argparse
import json
import subprocess
import sys
import time

REPO = "austinthedeveloper/picross-snes"


def gh(*args):
    r = subprocess.run(["gh", *args, "-R", REPO], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"gh {' '.join(args)}: {r.stderr.strip()}")
    return r.stdout


def open_prs():
    fields = "number,title,headRefName,baseRefName,mergeable,statusCheckRollup,isDraft"
    return json.loads(gh("pr", "list", "--state", "open", "--limit", "100", "--json", fields))


def checks(pr):
    states = [c.get("conclusion") or c.get("status") for c in pr.get("statusCheckRollup") or []]
    if not states:
        return "no checks"
    if all(s == "SUCCESS" for s in states):
        return "checks pass"
    if any(s in ("FAILURE", "CANCELLED", "TIMED_OUT") for s in states):
        return "checks FAIL"
    return "checks running"


def stacks(prs):
    by_head = {p["headRefName"]: p for p in prs}
    above = {}
    for p in prs:
        above.setdefault(p["baseRefName"], []).append(p)
    roots = [p for p in prs if p["baseRefName"] not in by_head]
    out = []
    for root in sorted(roots, key=lambda p: p["number"]):
        chain = [root]
        while len(above.get(chain[-1]["headRefName"], [])) == 1:
            chain.append(above[chain[-1]["headRefName"]][0])
        out.append(chain)
    return out


def show(prs):
    for chain in stacks(prs):
        print(f"stack on {chain[0]['baseRefName']}:")
        for p in chain:
            print(f"  #{p['number']:<4} {p['title'][:52]:52} {p['mergeable'].lower():12} {checks(p)}")
            print(f"        {p['headRefName']} -> {p['baseRefName']}")


def wait_mergeable(number):
    for _ in range(24):
        state = json.loads(gh("pr", "view", str(number), "--json", "mergeable"))["mergeable"]
        if state == "MERGEABLE":
            return
        if state == "CONFLICTING":
            sys.exit(f"#{number} has conflicts; resolve them and run again")
        time.sleep(5)
    sys.exit(f"#{number} isn't mergeable yet; try again shortly")


def merge(prs, top, delete, dry):
    chain = next((c for c in stacks(prs) if any(p["number"] == top for p in c)), None)
    if chain is None:
        sys.exit(f"#{top} isn't an open PR")
    chain = chain[:[p["number"] for p in chain].index(top) + 1]
    for p in chain:
        if checks(p) == "checks FAIL":
            sys.exit(f"#{p['number']}'s checks fail; not merging the stack")
    for p in chain:
        n = str(p["number"])
        if p["baseRefName"] != "main":
            print(f"#{n}: retarget {p['baseRefName']} -> main")
            if not dry:
                gh("pr", "edit", n, "--base", "main")
                time.sleep(3)
        print(f"#{n}: merge ({p['title']})")
        if not dry:
            wait_mergeable(p["number"])
            gh("pr", "merge", n, "--merge")
    if delete:
        for p in chain:
            print(f"delete branch {p['headRefName']}")
            if not dry:
                subprocess.run(["git", "push", "origin", "--delete", p["headRefName"]], check=False)
    print("done" if not dry else "dry run: nothing changed")


def strays():
    """Merged PRs whose head commit isn't in main: a stacked PR merged into
    the branch below it after that one had already merged."""
    subprocess.run(["git", "fetch", "-q", "origin"], check=True)
    merged = json.loads(gh("pr", "list", "--state", "merged", "--limit", "60", "--json",
                           "number,title,headRefName,baseRefName,headRefOid"))
    lost = []
    for p in merged:
        r = subprocess.run(["git", "merge-base", "--is-ancestor", p["headRefOid"], "origin/main"])
        if r.returncode:
            lost.append(p)
    if not lost:
        print("every merged PR is in main")
        return
    print("merged, but not in main:")
    for p in lost:
        print(f"  #{p['number']:<4} {p['title'][:50]:50} {p['headRefName']} -> {p['baseRefName']}")
    print("Open a PR to main from the newest branch that holds them all.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", nargs="?", choices=["show", "merge", "strays"], default="show")
    parser.add_argument("top", nargs="?", type=int)
    parser.add_argument("--delete-branches", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.command == "strays":
        strays()
        return
    prs = open_prs()
    if args.command == "show":
        show(prs)
        return
    if args.top is None:
        parser.error("merge needs the top PR's number")
    merge(prs, args.top, args.delete_branches, args.dry_run)


if __name__ == "__main__":
    main()
