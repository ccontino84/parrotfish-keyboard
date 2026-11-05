#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Count word tokens in sentence files.

Usage: count-words.py OUT.tsv FILE... [--exclude TEST_FILE]...

FILE is plain text (one sentence per line), or a Tatoeba export
(`id<TAB>lang<TAB>sentence`, optionally .bz2). Typographic apostrophes are
normalised to '. A trailing apostrophe stays part of the token, so the
Italian `perche'` is not counted as `perche`.

OUT has one line per lowercase word:
`word<TAB>count<TAB>cased<TAB>cased_count<TAB>inner_count`, where `cased` is
the most frequent spelling not at the start of a sentence, `cased_count`
its count there, and `inner_count` all occurrences not at the start.
"""

import argparse
import bz2
import collections
import re
import sys

TOKEN = re.compile(r"[^\W\d_]+(?:['\-][^\W\d_]+)*'?")


def lines(path):
    opener = bz2.open if path.endswith(".bz2") else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if path.endswith(".tsv") or path.endswith(".tsv.bz2"):
                parts = line.rstrip("\n").split("\t")
                if len(parts) < 3:
                    continue
                line = parts[2]
            yield line


def read_excluded(paths):
    """Sentences of the test sets (tests/data), left out of the counts."""
    out = set()
    for path in paths:
        with open(path, encoding="utf-8") as fh:
            out.update(l.strip() for l in fh if l.strip() and not l.startswith("#"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--exclude", action="append", default=[])
    args = ap.parse_args()
    out, files = args.out, args.files
    excluded = read_excluded(args.exclude)
    total = collections.Counter()
    cased = collections.Counter()
    for path in files:
        for line in lines(path):
            if line.strip() in excluded:
                continue
            line = line.replace("’", "'").replace("ʼ", "'")
            for i, t in enumerate(TOKEN.findall(line)):
                total[t.lower()] += 1
                if i > 0:
                    cased[t] += 1
    best = {}
    inner = collections.Counter()
    for t, c in cased.items():
        k = t.lower()
        inner[k] += c
        if k not in best or c > cased[best[k]]:
            best[k] = t
    with open(out, "w", encoding="utf-8") as fh:
        for w, c in total.most_common():
            b = best.get(w, w)
            fh.write(f"{w}\t{c}\t{b}\t{cased.get(b, 0)}\t{inner[w]}\n")
    print(f"{out}: {len(total)} types, {sum(total.values())} tokens")


if __name__ == "__main__":
    main()
