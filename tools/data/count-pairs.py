#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Count which words follow which, for the starting next-word data.

Usage: count-pairs.py OUT.tsv.gz WORDLIST.combined.gz SUPPLEMENT RULES_DIR FILE...
                      [--prev N] [--next K] [--min-count C] [--exclude TEST_FILE]...
                      [--respellings RESPELLINGS]

FILE is plain text (one sentence per line) or a Tatoeba export, as for
count-words.py. Words are compared in lowercase. For the N most frequent
words, OUT keeps the K most frequent next words that are in the wordlist
or the supplement (with the current spellings of RESPELLINGS, not the
old ones), as `prev<TAB>next<TAB>count`. Next words are written in
their most frequent spelling inside sentences (German `Dank`). `<s>` is
the start of a sentence, `<any>` the overall most frequent words (a fallback), and a line
`prev<TAB>*<TAB>total` gives the number of all continuations of `prev`.
Names in RULES_DIR/supplement.stop (corpus example names) are left out.
"""

import argparse
import bz2
import collections
import gzip
import re

TOKEN = re.compile(r"[^\W\d_]+(?:['\-][^\W\d_]+)*'?")
SENTENCE_END = re.compile(r"[.!?…]+[\"'»”)]*\s+(?=\S)")


def lines(path):
    opener = bz2.open if path.endswith(".bz2") else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if ".tsv" in path:
                parts = line.rstrip("\n").split("\t")
                if len(parts) < 3:
                    continue
                line = parts[2]
            yield line


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("wordlist")
    ap.add_argument("supplement")
    ap.add_argument("rules_dir")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--prev", type=int, default=20000)
    ap.add_argument("--next", type=int, default=12)
    ap.add_argument("--min-count", type=int, default=2)
    ap.add_argument("--exclude", action="append", default=[])
    ap.add_argument("--respellings", help="LANG.respellings of find-respellings.py")
    args = ap.parse_args()

    known = set()
    with gzip.open(args.wordlist, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                if fields.get("not_a_word") != "true" and int(fields["f"]) > 0:
                    known.add(fields["word"].lower())
    for line in read_list(args.supplement):
        known.add(line.split("\t")[0].lower())
    # Current spellings in, old ones out (find-respellings.py, as build-dict.py).
    for line in read_list(args.respellings) if args.respellings else []:
        old, new, kind = line.split("\t")[:3]
        known.add(new.lower())
        if kind == "old":
            known.discard(old.lower())
    stop = {w.lower() for w in read_list(f"{args.rules_dir}/supplement.stop")}
    known -= stop

    excluded = set()   # test sentences (tests/data), left out of the counts
    for path in args.exclude:
        with open(path, encoding="utf-8") as fh:
            excluded.update(l.strip() for l in fh if l.strip() and not l.startswith("#"))

    unigram = collections.Counter()
    cased = collections.Counter()   # spellings not at a sentence start
    pairs = collections.defaultdict(collections.Counter)
    for path in args.files:
        for line in lines(path):
            line = line.replace("’", "'").replace("ʼ", "'")
            for sentence in SENTENCE_END.split(line):
                prev = "<s>"
                for t in TOKEN.findall(sentence):
                    w = t.lower()
                    unigram[w] += 1
                    if prev != "<s>":
                        cased[t] += 1
                    pairs[prev][w] += 1
                    prev = w

    spelling = {}
    for t, c in cased.items():
        k = t.lower()
        if k not in spelling or c > cased[spelling[k]]:
            spelling[k] = t

    keep_prev = ["<s>"] + [w for w, _ in unigram.most_common() if w in known][:args.prev]
    with gzip.open(args.out, "wt", encoding="utf-8", compresslevel=9) as fh:
        top = [(w, c) for w, c in unigram.most_common(200) if w in known][:args.next * 2]
        fh.write(f"<any>\t*\t{sum(unigram.values())}\n")
        for w, c in top:
            fh.write(f"<any>\t{spelling.get(w, w)}\t{c}\n")
        for prev in keep_prev:
            nexts = pairs.get(prev)
            if not nexts:
                continue
            chosen = [(w, c) for w, c in nexts.most_common(args.next * 4)
                      if w in known and c >= args.min_count][:args.next]
            if not chosen:
                continue
            fh.write(f"{prev}\t*\t{sum(nexts.values())}\n")
            for w, c in chosen:
                fh.write(f"{prev}\t{spelling.get(w, w)}\t{c}\n")


if __name__ == "__main__":
    main()
