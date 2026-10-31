#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Mixed-language test texts for sentence-bench, from two sentence files.

Usage: mix-sentences.py KIND A.txt B.txt [-n 300] [-s SEED] > OUT.txt

KIND switch: a sentence of A, then a sentence of B, on one line (the text
changes language). KIND insert: a sentence of A with one word of B put in
at a random place (a foreign word inside a sentence: "ciao", "danke").
Each output line is one text for sentence-bench, which types it with the
earlier words as context; start it leaning towards A (-m A). The words of
B must not be changed by autocorrect: "correct changed" in the summary
counts them (with -r 0, only them).
"""
import argparse
import random
import re

WORD = re.compile(r"[^\W\d_]+(?:['\-][^\W\d_]+)*", re.UNICODE)


def sentences(path):
    with open(path, encoding="utf-8") as f:
        return [l.strip() for l in f if l.strip() and not l.startswith("#")]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kind", choices=["switch", "insert"])
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("-n", type=int, default=300)
    ap.add_argument("-s", type=int, default=1)
    args = ap.parse_args()
    rng = random.Random(args.s)
    a, b = sentences(args.a), sentences(args.b)
    for _ in range(args.n):
        first = rng.choice(a)
        if args.kind == "switch":
            print(first, rng.choice(b))
            continue
        words = WORD.findall(rng.choice(b))
        if not words:
            continue
        foreign = rng.choice(words)
        if foreign[0].isupper() and foreign.lower() != foreign:
            foreign = foreign.lower() if rng.random() < 0.5 else foreign
        parts = first.split(" ")
        at = rng.randint(1, len(parts)) if len(parts) > 1 else 1
        print(" ".join(parts[:at] + [foreign] + parts[at:]))


if __name__ == "__main__":
    main()
