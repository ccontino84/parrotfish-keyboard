#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Draw the test sentences of tests/data at random.

Usage: sample-sentences.py OUT.txt FILE... [--min-words N] [--max-words M]
                           [--count C] [--share S] [--seed SEED]

FILE is plain text (one sentence per line) or a Tatoeba export
(`id<TAB>lang<TAB>sentence`, optionally .bz2). Sentences with digits or
markup are skipped, and so are sentences outside MIN..MAX words. OUT gets
C sentences (--count), or fewer for a small corpus: at most --share of the
sentences that qualify, so most of the corpus is left for the word counts.
Sentences in OUT are left out of every count (`--exclude` in
tools/data/regenerate-lists.sh), so the benchmarks never test on data the
dictionaries were built from.
"""

import argparse
import bz2
import random
import re

MARKUP = re.compile(r"[\d<>\[\]{}#*_|/\\@=+~^$%&]")


def lines(path):
    opener = bz2.open if path.endswith(".bz2") else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if path.endswith(".tsv") or path.endswith(".tsv.bz2"):
                parts = line.rstrip("\n").split("\t")
                if len(parts) < 3:
                    continue
                line = parts[2]
            yield line.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--min-words", type=int, default=6)
    ap.add_argument("--max-words", type=int, default=25)
    ap.add_argument("--count", type=int, default=2000)
    ap.add_argument("--share", type=float, default=0.05)
    ap.add_argument("--seed", type=int, default=20261008)
    args = ap.parse_args()

    seen = set()
    pool = []
    for path in args.files:
        for s in lines(path):
            if not s or s in seen or MARKUP.search(s):
                continue
            if args.min_words <= len(s.split()) <= args.max_words:
                seen.add(s)
                pool.append(s)
    pool.sort()
    n = min(args.count, int(len(pool) * args.share))
    picked = random.Random(args.seed).sample(pool, n)
    with open(args.out, "w", encoding="utf-8") as fh:
        for s in picked:
            fh.write(s + "\n")
    print(f"{args.out}: {n} of {len(pool)} sentences")


if __name__ == "__main__":
    main()
