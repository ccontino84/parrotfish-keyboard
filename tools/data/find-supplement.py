#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
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
   so `the` doesn't become Italian, unless it is common in both sources
   (--other-rate per million, geometric mean): Danish and Spanish `vi`,
   Romanian `vom` are missing from their lists, while English `of` in
   Spanish Wikipedia sentences is rare in Tatoeba;
4. for languages without `capitalised-nouns` in LANG.rules, it is not
   mostly capitalised (a name);
5. it is not in RULES_DIR/supplement.stop (recurring example names of the
   Tatoeba corpus).

With `one-source N` in LANG.rules, a word seen at least N times in the two
sources together also counts for rule 1, even if one source lacks it: the
Finnish Common Voice text is too small to confirm most word forms. With
`one-source N tatoeba`, N times in Tatoeba alone (conversation that Common
Voice's read text lacks), its spellings without some accents counted with
its commonest one. Its
frequency comes from a fit over both sources counted as one.
`supplement-min-total N` in LANG.rules sets --min-total for a language
with little text (Romanian: niciuna 10 times). With
`no-misspellings`, å, ä, ö, æ and ø are letters of their own (vaara and
väärä are two words): rule 2 doesn't count them as accent variants.

Words below `min-freq N` in LANG.rules count as missing from the list
(build-dict.py leaves them out).

Its frequency is mapped to the AOSP 0..255 scale with a least-squares fit
of AOSP frequency on log(count) over words found in both. OUT lists
`word<TAB>f<TAB>evidence`; build-dict.py reads it. Review changes to OUT.
"""

import argparse
import gzip
import math
import sys
import unicodedata


OWN_LETTERS = "åäöæø"


def fold(word, keep=""):
    """Lowercase without accents; the letters in `keep` stay as they are."""
    s = "".join(c if c in keep else unicodedata.normalize("NFD", c) for c in word.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace("ß", "ss")


FINAL_APOSTROPHE_MIN = 20   # occurrences in each source of a word ending in '


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh
                    if l.split("#", 1)[0].strip()]
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
    ap.add_argument("--other-rate", type=float, default=30.0)
    ap.add_argument("--max-freq", type=int, default=170)
    args = ap.parse_args()

    rules = [l.split() for l in read_list(f"{args.rules_dir}/{args.lang}.rules")]
    capitalised_nouns = any(r[0] == "capitalised-nouns" for r in rules)
    one_source = max([int(r[1]) for r in rules if r[0] == "one-source"] or [0])
    for r in rules:
        if r[0] == "supplement-min-total":
            args.min_total = int(r[1])
    # `one-source N tatoeba`: only Tatoeba may be the one source (Spanish
    # déjame, cuéntame: conversation, rare in Common Voice's read text,
    # whose own words are encyclopaedic: demersal, óblast).
    tatoeba_only = any(r[0] == "one-source" and "tatoeba" in r[2:] for r in rules)
    keep = OWN_LETTERS if any(r[0] == "no-misspellings" for r in rules) else ""
    # Words ending in an apostrophe (Italian po', mo'): elsewhere a final
    # apostrophe is a closing quote or a plural possessive (dogs').
    final_apostrophe = any(r[0] == "final-apostrophe" for r in rules)
    # The vowel a final apostrophe stands for (perche' for perché), from
    # the rules: such a form is a misspelling, not a word.
    accent_rules = [(r[0][:-1], r[1]) for r in rules
                    if len(r) >= 2 and len(r[0]) == 2 and r[0].endswith("'") and len(r[1]) == 1]
    # Entries below `min-freq` are left out of the dictionary (build-dict.py):
    # here they count as missing.
    floor = max([int(r[1]) for r in rules if r[0] == "min-freq"] or [0])
    words = {w: f for w, f in read_wordlist(args.wordlist).items() if f >= floor}
    # Old spellings (idéias) are misspellings of their current forms
    # (ideias), which count as in the list.
    words = apply_respellings(words, args.rules_dir, args.lang)
    lower = {}
    for w, f in words.items():
        lower[w.lower()] = max(f, lower.get(w.lower(), 0))
    folded = {}
    for w in words:
        folded.setdefault(fold(w, keep), set()).add(w.lower())
    others = {}
    for path in args.other:
        for w, f in read_wordlist(path).items():
            others[w.lower()] = max(f, others.get(w.lower(), 0))
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

    def rate_one(w):
        """Per-million rate in both sources counted as one."""
        return (cv.get(w, (0,))[0] + tat.get(w, (0,))[0]) / (cv_total + tat_total) * 1e6

    def fit(rate):
        """f = a + b * ln(rate) over words in both the wordlist and the corpora."""
        xs, ys = [], []
        for w, f in lower.items():
            r = rate(w)
            if r > 0 and f > 0:
                xs.append(math.log(r))
                ys.append(f)
        n = len(xs)
        mx, my = sum(xs) / n, sum(ys) / n
        b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)
        return my - b * mx, b, n

    a, b, n = fit(rate)
    a1, b1, n1 = fit(rate_one) if one_source else (0, 0, 0)

    added, skipped = [], {"variant": 0, "other": 0, "name": 0, "stop": 0}
    none = (0, "", 0, 0)
    # One source: the commonest spelling of each word without accents in
    # Tatoeba, so that dejame (24) is not added beside déjame (366). Its
    # other spellings count as evidence that the word exists (olvídate 13,
    # olvidate 11: the accent is often left out).
    commonest, spellings = {}, {}
    if tatoeba_only:
        for k, v in tat.items():
            f = fold(k, keep)
            spellings[f] = spellings.get(f, 0) + v[0]
            if v[0] > commonest.get(f, (0, ""))[0]:
                commonest[f] = (v[0], k)
    for w in (set(cv) | set(tat)) if one_source else (set(cv) & set(tat)):
        c1, cased1, cc1, inner1 = cv.get(w, none)
        c2, cased2, cc2, inner2 = tat.get(w, none)
        both = c1 >= args.min_each and c2 >= args.min_each and c1 + c2 >= args.min_total
        if tatoeba_only and commonest.get(fold(w, keep), (0, ""))[1] == w:
            c_one = spellings[fold(w, keep)]
        else:
            c_one = c2 if tatoeba_only else c1 + c2
        if not both and not (one_source and c_one >= one_source):
            continue
        if not both and tatoeba_only and commonest[fold(w, keep)][1] != w \
                and commonest[fold(w, keep)][0] >= 3 * c2:
            skipped["variant"] += 1
            continue
        if len(w) < 2:
            continue
        if w.endswith("'"):
            # Common Voice also splits elided articles from their word (dell'
            # anno): such a form must be common in both sources.
            if not final_apostrophe or len(w) < 3 or min(c1, c2) < FINAL_APOSTROPHE_MIN or any(
                    w[-2] == v and (w[:-2] + a) in lower for v, a in accent_rules):
                continue
        # Spelling: the most frequent form inside sentences, in both sources.
        cased = cased1 if cc1 >= cc2 else cased2
        # Where nouns are capitalised, case tells words apart (dank, Dank):
        # compare the spelling; elsewhere, compare in lowercase.
        if (cased in words) if capitalised_nouns else (w in lower):
            continue
        if any(v != w for v in folded.get(fold(w, keep), ())):
            skipped["variant"] += 1
            continue
        if others.get(w, 0) >= args.other_freq and (rate(w) if both else rate_one(w)) < args.other_rate:
            skipped["other"] += 1
            continue
        if w in stop:
            skipped["stop"] += 1
            continue
        inner = inner1 + inner2
        if inner == 0:
            continue   # seen only at sentence starts: case unknown
        capital_share = (cc1 if cased1[:1].isupper() else 0) + (cc2 if cased2[:1].isupper() else 0)
        if cased[0].isupper() and not capitalised_nouns:
            skipped["name"] += 1
            continue
        if cased[0].isupper() and capital_share < 0.9 * inner and not capitalised_nouns:
            cased = w   # mixed case: keep it lowercase
        f = round(a + b * math.log(rate(w))) if both else round(a1 + b1 * math.log(rate_one(w)))
        added.append((cased, max(1, min(args.max_freq, f)), f"cv {c1} tatoeba {c2}"))

    added.sort(key=lambda x: (-x[1], x[0]))
    with open(args.out, "w", encoding="utf-8") as fh:
        # The header of the written file, split so that reuse doesn't take it as this file's.
        fh.write("# SPDX-" "FileCopyrightText: Mozilla Common Voice contributors, Tatoeba contributors\n"
                 "# SPDX-" "License-Identifier: LicenseRef-CC-BY-2.0-FR AND CC0-1.0\n"
                 "# Generated by tools/data/find-supplement.py from word counts of the Common Voice\n"
                 "# (CC0) and Tatoeba (CC BY 2.0 FR) sentences; see THIRD-PARTY.md. Review changes.\n"
                 f"# Frequency fit: f = {a:.1f} + {b:.2f} ln(rate per million), {n} words.\n"
                 + (f"# One source (one-source {one_source}): f = {a1:.1f} + {b1:.2f} ln(rate in both "
                    f"counted as one), {n1} words.\n" if one_source else "") +
                 "# word\tf\tevidence\n")
        for w, f, ev in added:
            fh.write(f"{w}\t{f}\t{ev}\n")
    print(f"{args.lang}: {len(added)} added; fit f = {a:.1f} + {b:.2f} ln(rate), "
          f"skipped " + ", ".join(f"{k} {v}" for k, v in skipped.items()), file=sys.stderr)


if __name__ == "__main__":
    main()
