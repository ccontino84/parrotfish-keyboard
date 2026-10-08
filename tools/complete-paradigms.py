#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Add the missing forms of regular verbs.

Usage: complete-paradigms.py LANG WORDLIST.combined.gz SUPPLEMENT COUNTS.tsv RULES_DIR OUT

The AOSP lists come from mostly third-person written text, so many verb
forms are missing (it: `riuscivi` next to `riuscivo`, `riusciva`; de:
`kommst`). A form that is missing from the list looks like a typo and gets
"corrected". RULES_DIR/LANG.paradigms gives the regular endings per verb
class and tense. A verb qualifies when:

1. its infinitive is in the list (lowercase, stem of 3+ letters), and so is
   its gerund (or present participle) unless the class has none;
2. it is not in RULES_DIR/LANG.paradigm-skip (irregular verbs).

In each tense with at least two attested forms, the missing forms are added,
unless they differ from an existing word only by accents (de: fahrst next to
fährst) or are listed in the skip file. When two classes share an
infinitive ending (it: dormire, finire), each tense takes the class with
more attested forms of its own.

COUNTS.tsv (count-words.py counts of proofread sentences) separates forms
seen in real text, which get a frequency from a fit like find-supplement.py,
from unseen ones, which get frequency 1: valid, never "corrected", never
suggested unprompted. OUT lists `word<TAB>f<TAB>evidence`; build-dict.py
reads it. Review a sample of OUT when it changes.
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
            return [l.split("#", 1)[0].strip() for l in fh if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def read_paradigms(path):
    spell, classes = set(), []
    for line in read_list(path):
        parts = line.split()
        if parts[0] == "spell":
            spell.add(parts[1] + ("" if len(parts) < 3 else ":" + parts[2]))
        elif parts[0] == "class":
            cls = {"name": parts[1], "inf": parts[2], "gerund": parts[3] if len(parts) > 3 else None,
                   "stem": None, "tenses": {}}
            for p in parts[4:]:
                if p.startswith("stem="):
                    cls["stem"] = tuple(p[5:].split("|"))
            classes.append(cls)
        elif parts[0] == "tense":
            name = parts[1].rstrip(":")
            classes[-1]["tenses"][name] = parts[2:]
    return spell, classes


def join(stem, ending, cls, spell):
    """stem + ending with the language's spelling adjustments."""
    if not ending:
        return stem
    e0 = ending[0]
    if f"h-after-cg-before-ei:{cls['name']}" in spell and stem[-1:] in ("c", "g") and e0 in "eèéiì":
        return stem + "h" + ending
    if "drop-i-before-i" in spell and stem.endswith("i") and e0 in "iì":
        return stem + ending[1:] if len(ending) > 1 else stem
    if "drop-i-after-cg-before-e" in spell and stem[-2:] in ("ci", "gi") and e0 in "eèé":
        return stem[:-1] + ending
    if "e-after-dt-before-st-t" in spell and stem[-1:] in ("d", "t") and ending[:2] in ("st", "t", "te"):
        return stem + "e" + ending
    if "drop-s-after-sz-before-st" in spell and stem[-1:] in ("s", "ß", "z", "x") and ending == "st":
        return stem + "t"
    return stem + ending


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("supplement")
    ap.add_argument("counts")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    args = ap.parse_args()

    spell, classes = read_paradigms(f"{args.rules_dir}/{args.lang}.paradigms")
    if not classes:
        with open(args.out, "w", encoding="utf-8") as fh:
            fh.write("# SPDX-FileCopyrightText: 2026 ccontino84\n# SPDX-License-Identifier: LicenseRef-TBD\n"
                     "# No paradigms for this language.\n")
        return

    words = {}
    with gzip.open(args.wordlist, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                words[fields["word"]] = max(int(fields["f"]), words.get(fields["word"], 0))
    for line in read_list(args.supplement):
        w, f = line.split("\t")[:2]
        words.setdefault(w, int(f))
    known = set(words)
    folded = {}
    for w in words:
        folded.setdefault(fold(w), set()).add(w)
    skip = set(read_list(f"{args.rules_dir}/{args.lang}.paradigm-skip"))

    counts = {}
    with open(args.counts, encoding="utf-8") as fh:
        for line in fh:
            w, c = line.rstrip("\n").split("\t")[:2]
            counts[w] = int(c)
    total = sum(counts.values())
    # Fit f = a + b ln(rate) over words in both, for forms seen in the corpus.
    xs, ys = [], []
    for w, f in words.items():
        c = counts.get(w.lower(), 0)
        if c > 0 and f > 0:
            xs.append(math.log(c / total * 1e6))
            ys.append(f)
    mx, my = sum(xs) / len(xs), sum(ys) / len(ys)
    b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)
    a = my - b * mx

    generated = {}
    verbs = 0
    infinitives = sorted({c["inf"] for c in classes})
    for w in sorted(words):
        if w != w.lower() or w in skip:
            continue
        for inf_ending in infinitives:
            if not w.endswith(inf_ending) or len(w) - len(inf_ending) < 3:
                continue
            stem = w[:-len(inf_ending)]
            group = [c for c in classes if c["inf"] == inf_ending
                     and (c["stem"] is None or stem.endswith(c["stem"]))]
            group = [c for c in group if c["gerund"] is None
                     or join(stem, c["gerund"], c, spell) in known]
            if not group:
                continue
            verbs += 1
            tense_names = {t for c in group for t in c["tenses"]}
            for tense in tense_names:
                options = []
                for c in group:
                    if tense not in c["tenses"]:
                        continue
                    forms = [join(stem, e, c, spell) for e in c["tenses"][tense]]
                    others = {join(stem, e, o, spell) for o in group if o is not c
                              for e in o["tenses"].get(tense, [])}
                    attested = {f for f in forms if f in known}
                    own = {f for f in attested if f not in others}
                    options.append((len(own), len(attested), c, forms, attested))
                if not options:
                    continue
                options.sort(key=lambda o: (o[0], o[1]), reverse=True)
                own, attested_n, c, forms, attested = options[0]
                if attested_n < 2 or (len(options) > 1 and own == 0):
                    continue
                for f in forms:
                    if f in known or f in skip or f in generated:
                        continue
                    if fold(f) in folded:
                        continue   # an accent variant of an existing word
                    seen = counts.get(f, 0)
                    freq = max(1, min(150, round(a + b * math.log(seen / total * 1e6)))) if seen else 1
                    generated[f] = (freq, f"{w} {tense} ({c['name']}), corpus {seen}")

    out = sorted(generated.items(), key=lambda kv: kv[0])
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("# SPDX-FileCopyrightText: 2026 ccontino84\n"
                 "# SPDX-License-Identifier: LicenseRef-TBD\n"
                 "# Generated by tools/complete-paradigms.py; review a sample when it changes.\n"
                 "# word\tf\tevidence (infinitive, tense, class, corpus count)\n")
        for w, (f, ev) in out:
            fh.write(f"{w}\t{f}\t{ev}\n")
    seen_n = sum(1 for _, (f, ev) in out if not ev.endswith("corpus 0"))
    print(f"{args.lang}: {verbs} verbs, {len(out)} forms added ({seen_n} seen in the corpus)",
          file=sys.stderr)


if __name__ == "__main__":
    main()
