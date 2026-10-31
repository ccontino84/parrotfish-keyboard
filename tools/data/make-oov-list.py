#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""List real words that a dictionary doesn't know, for typo-bench -o.

Usage: make-oov-list.py COUNTS.tsv WORDLIST.combined.gz SUPPLEMENT OUT [--min-count N]

COUNTS.tsv comes from count-words.py over proofread sentences. A word that
occurs at least N times there but is in neither the wordlist nor the
supplement is a real word the user may type (a name, a rare word, a new
word): autocorrect must leave it alone.
"""

import argparse
import gzip
import re


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("counts")
    ap.add_argument("wordlist")
    ap.add_argument("supplement")
    ap.add_argument("out")
    ap.add_argument("--min-count", type=int, default=5)
    args = ap.parse_args()

    known = set()
    with gzip.open(args.wordlist, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                known.add(line[6:line.index(",")].lower())
    with open(args.supplement, encoding="utf-8") as fh:
        for line in fh:
            if not line.startswith("#"):
                known.add(line.split("\t")[0].lower())

    out = []
    with open(args.counts, encoding="utf-8") as fh:
        for line in fh:
            parts = line.rstrip("\n").split("\t")
            w, c = parts[0], int(parts[1])
            if c < args.min_count:
                break
            cased = parts[2] if len(parts) > 2 else w
            if len(w) >= 3 and w not in known and re.fullmatch(r"[^\W\d_]+", w):
                out.append(cased)
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out) + "\n")
    print(f"{args.out}: {len(out)} words")


if __name__ == "__main__":
    main()
