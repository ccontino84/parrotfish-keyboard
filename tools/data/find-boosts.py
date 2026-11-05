#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Calibrate word frequencies towards everyday language, up and down.

Usage: find-boosts.py LANG WORDLIST.combined.gz SUPPLEMENT TATOEBA.tsv RULES_DIR OUT
                      [--up-half N] [--down-half N] [--margin STEPS]

The AOSP frequencies come largely from encyclopedic text: in Italian
`pensò` (past historic) outranks `penso`, and in English `in` (f 210) is
e^5.6 times likelier than `I'm` (116), while in conversation it is 3-9
times. TATOEBA.tsv (count-words.py counts of the Tatoeba sentences, CC BY
2.0 FR) is everyday language.

Rank mapping: over the words of the list that Tatoeba has, the n-th
commonest there gets the n-th highest frequency of those same words (tied
counts: the mean of their places). The frequencies as a whole stay the
same, so do the language's total probability and the scale against
"a word missing from the dictionaries"; only which word has which changes.
A word moves the share c / (c + N) of the way to its target, c its
Tatoeba count: up with N = --up-half (3: a word seen once or twice is
noise, English `ciao` twice), down with N = --down-half (10000): only
words Tatoeba has many thousand times (in, of, and, by: written-text
function words) come down fully. Lowering mid-frequency written words
(during, described) made real misspellings of them stay unfixed and did
nothing for conversational text. Words Tatoeba lacks keep their
frequency and their order.

Words are matched in lowercase (the count covers every case spelling); the
spelling the corpus uses inside sentences moves, if the list has it.
Left alone: words the counter cannot see as one token (digits, dots),
the generated verb forms (complete-paradigms.py counts them already) and
the names in RULES_DIR/supplement.stop. As in build-dict.py, `min-freq N`
in LANG.rules drops the rare entries first. OUT lists
`word<TAB>f<TAB>evidence` for moves of at least --margin steps;
build-dict.py sets those frequencies.
"""

import argparse
import gzip
import re
import sys

# The tokens count-words.py counts.
TOKEN = re.compile(r"[^\W\d_]+(?:['\-][^\W\d_]+)*'?")


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def apply_respellings(words, rules_dir, lang):
    """The list as build-dict.py makes it with RULES_DIR/LANG.respellings
    (find-respellings.py): current spellings in, with at least the old
    one's frequency; old ones out (kind `old`, not `both`)."""
    for line in read_list(f"{rules_dir}/{lang}.respellings"):
        old, new, kind = line.split("\t")[:3]
        f = words.pop(old, 0) if kind == "old" else words.get(old, 0)
        if f or new in words:
            words[new] = max(f, words.get(new, 0))
    return words


def calibrate(words, counts, keys, up_half, down_half):
    """{spelling: (old f, new f, count, target)} for every key Tatoeba has.

    words: {spelling: f}; counts: {lowercase: (count, cased spelling)};
    keys: the lowercase keys that may move."""
    by_lower = {}
    for w in words:
        by_lower.setdefault(w.lower(), []).append(w)

    def spelling(k):
        forms = by_lower[k]
        cased = counts[k][1]
        return cased if cased in forms else (k if k in forms else max(forms, key=lambda x: words[x]))

    seen = sorted(((counts[k][0], spelling(k)) for k in keys if k in counts and counts[k][0] > 0),
                  key=lambda x: -x[0])
    fs = sorted((words[w] for _, w in seen), reverse=True)
    out = {}
    i = 0
    while i < len(seen):
        j = i
        while j < len(seen) and seen[j][0] == seen[i][0]:
            j += 1
        target = sum(fs[i:j]) / (j - i)
        for c, w in seen[i:j]:
            f = words[w]
            share = c / (c + (up_half if target >= f else down_half))
            out[w] = (f, max(1, min(255, round(f + share * (target - f)))), c, target)
        i = j
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("supplement")
    ap.add_argument("tatoeba")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    ap.add_argument("--up-half", type=float, default=3.0)
    ap.add_argument("--down-half", type=float, default=10000.0)
    ap.add_argument("--margin", type=int, default=2)
    args = ap.parse_args()

    words = {}
    with gzip.open(args.wordlist, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                if fields.get("not_a_word") == "true" or int(fields["f"]) == 0:
                    continue
                words[fields["word"]] = max(int(fields["f"]), words.get(fields["word"], 0))
    for line in read_list(f"{args.rules_dir}/{args.lang}.rules"):
        if line.split()[0] == "min-freq":
            words = {w: f for w, f in words.items() if f >= int(line.split()[1])}
    for line in read_list(args.supplement):
        w, f = line.split("\t")[:2]
        words.setdefault(w, int(f))
    # As build-dict.py: old spellings out, their current forms in.
    words = apply_respellings(words, args.rules_dir, args.lang)
    stop = {w.lower() for w in read_list(f"{args.rules_dir}/supplement.stop")}

    counts = {}
    with open(args.tatoeba, encoding="utf-8") as fh:
        for line in fh:
            w, c, cased = line.rstrip("\n").split("\t")[:3]
            counts[w] = (int(c), cased)
    total = sum(c for c, _ in counts.values())

    keys = {w.lower() for w in words} - stop
    keys = [k for k in keys if TOKEN.fullmatch(k)]
    moved = calibrate(words, counts, keys, args.up_half, args.down_half)
    out = sorted(((w, new, f"f {f} tatoeba {c} target {t:.0f}") for w, (f, new, c, t) in moved.items()
                  if abs(new - f) >= args.margin), key=lambda x: (-x[1], x[0]))
    raised = sum(1 for w, new, _ in out if new > words[w])

    with open(args.out, "w", encoding="utf-8") as fh:
        # The header of the written file, split so that reuse doesn't take it as this file's.
        fh.write("# SPDX-" "FileCopyrightText: Tatoeba contributors\n"
                 "# SPDX-" "License-Identifier: LicenseRef-CC-BY-2.0-FR\n"
                 "# Generated by tools/data/find-boosts.py from Tatoeba word counts (CC BY 2.0 FR);\n"
                 "# see THIRD-PARTY.md. Review changes.\n"
                 f"# Rank mapping over {len(moved)} words seen in {total} tokens; up-half {args.up_half:g}, "
                 f"down-half {args.down_half:g}, "
                 f"margin {args.margin}.\n"
                 "# word\tf\tevidence (old f, Tatoeba count, rank target)\n")
        for w, f, ev in out:
            fh.write(f"{w}\t{f}\t{ev}\n")
    print(f"{args.lang}: {raised} words raised, {len(out) - raised} lowered", file=sys.stderr)


if __name__ == "__main__":
    main()
