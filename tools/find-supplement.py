#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Find common words missing from an AOSP wordlist.

Usage: find-supplement.py LANG WORDLIST.combined.gz CV.tsv TATOEBA.tsv RULES_DIR OUT
                          [--other OTHER_WORDLIST.combined.gz]...

The AOSP lists have gaps: the German one has only 63 words starting with a
lowercase umlaut (überhaupt, ändern are missing), the Italian one lacks
`uomo`, and conversational forms (sprichst, volevate) are rare in them.
CV.tsv and TATOEBA.tsv are count-words.py counts of the Common Voice (CC0)
and Tatoeba (CC BY 2.0 FR) sentences. A word is added when:

1. it occurs in both sources (--min-each) and often enough (--min-total);
2. it is not in the list yet (for languages with `capitalised-nouns`, in
   that spelling: the list has `dank` but not the noun `Dank`), and not an
   accent or ß variant of a word already in it (daß);
3. it is not a common word of another language (--other, --other-freq),
   so `the` doesn't become Italian;
4. for languages without `capitalised-nouns` in LANG.rules, it is not
   mostly capitalised (a name);
5. it is not in RULES_DIR/supplement.stop (recurring example names of the
   Tatoeba corpus).

Its frequency is mapped to the AOSP 0..255 scale with a least-squares fit
of AOSP frequency on log(count) over words found in both. OUT lists
`word<TAB>f<TAB>evidence`; build-dict.py reads it. Review changes to OUT.
"""

import argparse
import gzip
import math
import sys
import unicodedata


def fold(word):
    s = unicodedata.normalize("NFD", word.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace("ß", "ss")


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh
                    if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def read_wordlist(path):
    words = {}
    with gzip.open(path, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                w = fields["word"]
                words[w] = max(int(fields["f"]), words.get(w, 0))
    return words


def read_counts(path):
    counts = {}
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            w, c, cased, cc, inner = line.rstrip("\n").split("\t")
            counts[w] = (int(c), cased, int(cc), int(inner))
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("cv")
    ap.add_argument("tatoeba")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    ap.add_argument("--other", action="append", default=[])
    ap.add_argument("--min-each", type=int, default=3)
    ap.add_argument("--min-total", type=int, default=20)
    ap.add_argument("--other-freq", type=int, default=90)
    ap.add_argument("--max-freq", type=int, default=170)
    args = ap.parse_args()

    words = read_wordlist(args.wordlist)
    lower = {}
    for w, f in words.items():
        lower[w.lower()] = max(f, lower.get(w.lower(), 0))
    folded = {}
    for w in words:
        folded.setdefault(fold(w), set()).add(w.lower())
    others = {}
    for path in args.other:
        for w, f in read_wordlist(path).items():
            others[w.lower()] = max(f, others.get(w.lower(), 0))
    rules = [l.split() for l in read_list(f"{args.rules_dir}/{args.lang}.rules")]
    capitalised_nouns = any(r[0] == "capitalised-nouns" for r in rules)
    stop = {w.lower() for w in read_list(f"{args.rules_dir}/supplement.stop")}

    cv = read_counts(args.cv)
    tat = read_counts(args.tatoeba)
    cv_total = sum(c[0] for c in cv.values())
    tat_total = sum(c[0] for c in tat.values())

    def rate(w):
        """Geometric mean of the per-million rates in both sources."""
        a = cv.get(w, (0,))[0] / cv_total * 1e6
        b = tat.get(w, (0,))[0] / tat_total * 1e6
        return math.sqrt(a * b)

    # Fit f = a + b * ln(rate) over words in both the wordlist and the corpora.
    xs, ys = [], []
    for w, f in lower.items():
        r = rate(w)
        if r > 0 and f > 0:
            xs.append(math.log(r))
            ys.append(f)
    n = len(xs)
    mx, my = sum(xs) / n, sum(ys) / n
    b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)
    a = my - b * mx

    added, skipped = [], {"variant": 0, "other": 0, "name": 0, "stop": 0}
    for w in set(cv) & set(tat):
        c1, cased1, _, inner1 = cv[w]
        c2, cased2, cc2, inner2 = tat[w]
        if c1 < args.min_each or c2 < args.min_each or c1 + c2 < args.min_total:
            continue
        if len(w) < 2 or w.endswith("'"):
            continue
        # Spelling: the most frequent form inside sentences, in both sources.
        cased = cased1 if cv[w][2] >= cc2 else cased2
        # Where nouns are capitalised, case tells words apart (dank, Dank):
        # compare the spelling; elsewhere, compare in lowercase.
        if (cased in words) if capitalised_nouns else (w in lower):
            continue
        if any(v != w for v in folded.get(fold(w), ())):
            skipped["variant"] += 1
            continue
        if others.get(w, 0) >= args.other_freq:
            skipped["other"] += 1
            continue
        if w in stop:
            skipped["stop"] += 1
            continue
        inner = inner1 + inner2
        if inner == 0:
            continue   # seen only at sentence starts: case unknown
        capital_share = (cv[w][2] if cased1[0].isupper() else 0) + (cc2 if cased2[0].isupper() else 0)
        if cased[0].isupper() and not capitalised_nouns:
            skipped["name"] += 1
            continue
        if cased[0].isupper() and capital_share < 0.9 * inner and not capitalised_nouns:
            cased = w   # mixed case: keep it lowercase
        f = max(1, min(args.max_freq, round(a + b * math.log(rate(w)))))
        added.append((cased, f, f"cv {c1} tatoeba {c2}"))

    added.sort(key=lambda x: (-x[1], x[0]))
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("# SPDX-FileCopyrightText: Mozilla Common Voice contributors, Tatoeba contributors\n"
                 "# SPDX-License-Identifier: LicenseRef-CC-BY-2.0-FR AND CC0-1.0\n"
                 "# Generated by tools/find-supplement.py from word counts of the Common Voice\n"
                 "# (CC0) and Tatoeba (CC BY 2.0 FR) sentences; see THIRD-PARTY.md. Review changes.\n"
                 f"# Frequency fit: f = {a:.1f} + {b:.2f} ln(rate per million), {n} words.\n"
                 "# word\tf\tevidence\n")
        for w, f, ev in added:
            fh.write(f"{w}\t{f}\t{ev}\n")
    print(f"{args.lang}: {len(added)} added; fit f = {a:.1f} + {b:.2f} ln(rate), "
          f"skipped " + ", ".join(f"{k} {v}" for k, v in skipped.items()), file=sys.stderr)


if __name__ == "__main__":
    main()
