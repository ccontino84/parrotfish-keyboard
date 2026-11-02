#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Find accent-less misspellings in an AOSP wordlist.

Usage: find-accent-misspellings.py LANG WORDLIST.combined.gz COUNTS.tsv RULES_DIR OUT

The AOSP lists contain common accent-less misspellings as normal words
(it: perche, de: Strasse). A plain spelling is marked as a misspelling of
its accented form only when all of these hold:

1. the accented form is much more frequent in the wordlist (--dict-steps);
2. in proofread sentences (COUNTS.tsv, from count-words.py) the accented
   form is common and the plain one rare (--min-count, --ratio);
3. no paradigm rule in RULES_DIR/LANG.rules says the plain form is a real
   inflection (it: `collaboro` stays, because `collaborare` exists);
   A `paradigm-plain` rule does the same from the plain form's ending
   (es: `acelero` stays, because `acelerar` exists: "I accelerate");
4. if LANG.rules has `only-marks CHARS`, the pair differs in one of them
   (de: `only-marks ß`, since many umlaut pairs are two real words:
   mochte/möchte, bluten/Blüten); `no-misspellings` marks none (sv, da,
   nb: å, ä, ö, æ, ø are letters of their own, har/här are two words);
5. the plain form is not an acronym (dell'ETA) and not in RULES_DIR/LANG.keep.

The list is read as build-dict.py makes it with LANG.respellings: an old
spelling (idéia) is a misspelling of the current one already.

When unsure, the word stays valid and the accented form is only offered as
a suggestion. OUT lists `plain<TAB>accented<TAB>evidence`; build-dict.py
reads it. Review the diff of OUT when it changes.
"""

import argparse
import gzip
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
            if not line.startswith(" word="):
                continue
            fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
            if fields.get("not_a_word") == "true":
                continue
            w, f = fields["word"], int(fields["f"])
            words[w] = max(f, words.get(w, 0))
    return words


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("counts")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    ap.add_argument("--dict-steps", type=int, default=20)
    ap.add_argument("--min-count", type=int, default=20)
    ap.add_argument("--ratio", type=float, default=30.0)
    args = ap.parse_args()

    words = read_wordlist(args.wordlist)
    lower = {w.lower() for w in words}
    counts = {}
    with open(args.counts, encoding="utf-8") as fh:
        for line in fh:
            w, c = line.rstrip("\n").split("\t")[:2]
            counts[w] = int(c)

    # `paradigm ò are` : a plain form X+o is real if X+are is a known word.
    paradigms = []
    plain_paradigms = []
    only_marks = None
    marking = True
    for line in read_list(f"{args.rules_dir}/{args.lang}.rules"):
        parts = line.split()
        if parts[0] == "paradigm":
            paradigms.append((parts[1], parts[2:]))
        elif parts[0] == "paradigm-plain":
            plain_paradigms.append((parts[1], parts[2:]))
        elif parts[0] == "no-misspellings":
            marking = False
        elif parts[0] == "only-marks":
            only_marks = set("".join(parts[1:]))
        elif parts[0] == "min-freq":
            # build-dict.py leaves rarer entries out
            words = {w: f for w, f in words.items() if f >= int(parts[1])}
    # Old spellings are misspellings of the current ones already.
    words = apply_respellings(words, args.rules_dir, args.lang)
    lower = {w.lower() for w in words}
    keep = set(read_list(f"{args.rules_dir}/{args.lang}.keep"))

    def known(w):
        return w in lower or counts.get(w, 0) > 0

    by_fold = {}
    for w in words:
        by_fold.setdefault(fold(w), []).append(w)

    marked, skipped = [], {"keep": 0, "marks": 0, "dict": 0, "count": 0, "ratio": 0, "paradigm": 0}
    for group in by_fold.values():
        accented = [w for w in group if fold(w) != w.lower()]
        plain = [w for w in group if fold(w) == w.lower()]
        if not accented or not plain:
            continue
        acc = max(accented, key=lambda w: words[w])
        ca = counts.get(acc.lower(), 0)
        for p in plain:
            cp = counts.get(p.lower(), 0)
            if p in keep or sum(c.isupper() for c in p) > 1:
                skipped["keep"] += 1
            elif only_marks is not None and not only_marks & set(acc.lower()):
                skipped["marks"] += 1
            elif words[acc] - words[p] < args.dict_steps:
                skipped["dict"] += 1
            elif ca < args.min_count:
                skipped["count"] += 1
            elif cp * args.ratio > ca:
                skipped["ratio"] += 1
            elif any(acc.endswith(end) and any(known(acc[:-len(end)].lower() + s) for s in stems)
                     for end, stems in paradigms) \
                    or any(p.lower().endswith(end) and any(known(p[:-len(end)].lower() + s) for s in stems)
                           for end, stems in plain_paradigms):
                skipped["paradigm"] += 1
            elif not marking:
                skipped["marks"] += 1
            else:
                marked.append((p, acc, f"dict {words[p]}/{words[acc]} corpus {cp}/{ca}"))

    marked.sort(key=lambda m: fold(m[0]))
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write("# SPDX-FileCopyrightText: 2026 ccontino84\n"
                 "# SPDX-License-Identifier: LicenseRef-TBD\n"
                 "# Generated by tools/data/find-accent-misspellings.py; review changes.\n"
                 "# plain\taccented\tevidence (wordlist f, corpus count: plain/accented)\n")
        for m in marked:
            fh.write("\t".join(m) + "\n")
    print(f"{args.lang}: {len(marked)} marked; not marked: " +
          ", ".join(f"{k} {v}" for k, v in skipped.items()), file=sys.stderr)


if __name__ == "__main__":
    main()
