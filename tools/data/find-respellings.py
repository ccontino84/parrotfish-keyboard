#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Find outdated spellings and their current form, from spelling-reform rules.

Usage: find-respellings.py LANG WORDLIST.combined.gz COUNTS.tsv RULES_DIR OUT
                           [--ratio R] [--min-count N]

Some wordlists predate a spelling reform: the Brazilian Portuguese one has
idéia, vôo, cinqüenta (2009 reform: ideia, voo, cinquenta), so autocorrect
changed the current spelling into the old one. And the sentence corpora
hold older texts (German daß, Danish paa), whose spellings are in no list.
`respell PATTERN REPLACEMENT [FLAGS]` lines in RULES_DIR/LANG.rules give a
reform's changes as a Python regular expression and its replacement
(`respell ôo oo always`). A word the pattern matches, written the new way,
is its current spelling. Flags:

- `list`: words of the wordlist may be outdated (the list predates the
  reform). Without it only words missing from the list are, when the list
  has their current form (German: the list is current, so Fuß, Floß stay;
  the corpus's daß, Fluß are old forms of dass, Fluss).
- `always`: the reform has no exceptions in lowercase words, so no corpus
  evidence is needed (the 0/0 of enjôo/enjoo), unless the corpus prefers
  the old form (seen --min-count times, more than the new one).
  Capitalised words (names: Agüero) still need the evidence.
- `lower`: only words written in lowercase (Danish aa → å: Aalborg,
  Aarhus keep their official aa).
- `both`: both spellings are in use (European Portuguese acção, ação):
  a current form the list lacks is added (seen --min-count times), the
  old one stays valid.

`respell-accent SUFFIXES PATTERN` (Spanish imperatives with pronouns:
muestrame, hablame in the list, muéstrame, háblame in use): a word of the
list without accents that ends in one of SUFFIXES (a regular expression)
after a stem that is itself a word of the list (muestra + me) is an old
form of the spelling with accents the corpus uses, if that spelling matches
PATTERN (where the accent falls) and the corpus evidence holds. The stem
check keeps other words (titulo, calculo: verb forms beside título,
cálculo).

The corpus evidence (COUNTS.tsv from count-words.py over proofread
sentences, lowercase): the current form is seen at least --min-count times
and --ratio times as often as the old one. Words in RULES_DIR/LANG.keep
stay as they are.

OUT lists `old<TAB>current<TAB>kind<TAB>evidence`. build-dict.py adds the
current form with the old one's frequency if the list lacks it (or raises
it to it) and, for kind `old`, marks the old form as a misspelling of the
current one (added if the list lacks it). Review the diff of OUT when it
changes.
"""

import argparse
import gzip
import re
import sys
import unicodedata


def fold(word):
    """Lowercase, without accents, ß as ss."""
    s = unicodedata.normalize("NFD", word.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace("ß", "ss")


def marks(word):
    """The number of accents and other marks of a word."""
    return sum(1 for c in unicodedata.normalize("NFD", word) if unicodedata.category(c) == "Mn") \
        + word.count("ß")


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh
                    if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def read_wordlist(path):
    """{word: f}, without the list's own misspellings."""
    words, missp = {}, set()
    with gzip.open(path, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                if fields.get("not_a_word") == "true":
                    missp.add(fields["word"])
                    continue
                words[fields["word"]] = max(int(fields["f"]), words.get(fields["word"], 0))
    return {w: f for w, f in words.items() if w not in missp}, missp


def read_rules(path):
    """[(compiled pattern, replacement, flags)] from the `respell` lines,
    and the `min-freq` floor."""
    rules, floor, accents = [], 0, []
    for line in read_list(path):
        parts = line.split()
        if parts[0] == "respell-accent":
            if len(parts) != 3:
                sys.exit(f"{path}: bad respell-accent rule {line!r}")
            accents.append((re.compile(f"(?:{parts[1]})"), re.compile(parts[2])))
        elif parts[0] == "respell":
            if len(parts) < 3:
                sys.exit(f"{path}: bad respell rule {line!r}")
            rules.append((re.compile(parts[1]), parts[2], set(parts[3:])))
        elif parts[0] == "min-freq":
            floor = int(parts[1])
    return rules, floor, accents


def respell(rules, word, in_list, both):
    """The word with every applicable rule applied (the `both` rules or the
    others), and the flags of the rules that changed it (None if none did)."""
    new, flags = word, None
    for pattern, repl, f in rules:
        if in_list and "list" not in f or ("both" in f) != both:
            continue
        if "lower" in f and not word[:1].islower():
            continue
        changed = pattern.sub(repl, new)
        if changed != new:
            new = changed
            flags = (flags or set()) | f
    return new, flags


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("counts")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    ap.add_argument("--ratio", type=float, default=3.0)
    ap.add_argument("--min-count", type=int, default=3)
    ap.add_argument("-v", "--verbose", action="store_true", help="print the candidates left out")
    args = ap.parse_args()

    rules, floor, accents = read_rules(f"{args.rules_dir}/{args.lang}.rules")
    words, missp = read_wordlist(args.wordlist)
    words = {w: f for w, f in words.items() if f >= floor}
    missp |= {l.split("\t")[0] for l in read_list(f"{args.rules_dir}/{args.lang}.manual-misspellings")}
    keep = set(read_list(f"{args.rules_dir}/{args.lang}.keep"))
    lower = {}
    for w in words:
        lower.setdefault(w.lower(), []).append(w)
    counts = {}
    with open(args.counts, encoding="utf-8") as fh:
        for line in fh:
            parts = line.rstrip("\n").split("\t")
            counts[parts[0]] = (int(parts[1]), parts[2] if len(parts) > 2 else parts[0])

    def count(w):
        return counts.get(w.lower(), (0,))[0]

    folded = {}
    for w in words:
        folded.setdefault(fold(w), set()).add(w)

    found, skipped = {}, {"evidence": 0, "keep": 0, "no-current": 0, "variant": 0}
    # Words of the list, then words of the corpus the list lacks (in the
    # spelling the corpus uses inside sentences).
    candidates = [(w, True) for w in words]
    candidates += [(cased, False) for k, (c, cased) in counts.items()
                   if k not in lower and cased not in missp]
    for (old, in_list), both in [(c, b) for b in (False, True) for c in candidates]:
        if not rules or old in missp or old in found:
            continue
        new, flags = respell(rules, old, in_list, both)
        if flags is None:
            continue
        if old in keep:
            skipped["keep"] += 1
            if args.verbose:
                print(f"keep\t{old}\t{new}", file=sys.stderr)
            continue
        if both and (not in_list or new in words or new.lower() in lower):
            continue            # only adds a missing current form
        if new in words:
            current = new
        elif in_list:
            current = None      # added as it is
        else:
            # A corpus word: the list's spelling (Fluß -> Fluss), or in
            # lowercase if the list has only that (Assembléia -> assembleia).
            current = None
            if new.lower() in lower and new.lower() in words and old.lower() not in lower:
                old, current = old.lower(), new.lower()
            if current is None:
                skipped["no-current"] += 1   # nothing to correct it to
                if args.verbose:
                    print(f"no-current\t{old}\t{new}", file=sys.stderr)
                continue
            new = current
        top = max(words.get(old, 0), words.get(new, 0))
        if not both and any(w not in (old, new) and words[w] >= top and marks(w) > marks(new)
                            for w in folded.get(fold(new), ())):
            # A commoner word of the list is spelled so with more accents:
            # the old form is a misspelling of that (algúem, heróina: alguém,
            # heroína; not alguem, heroina). Fewer accents is another word
            # (müßt: müsst, though musst is commoner).
            skipped["variant"] += 1
            if args.verbose:
                print(f"variant\t{old}\t{new}", file=sys.stderr)
            continue
        co, cn = count(old), count(new)
        evident = cn >= args.min_count and (both or cn >= args.ratio * co)
        # `always`: unless the corpus clearly prefers the old form.
        if not evident and (both or not ("always" in flags and old[:1].islower()
                                         and (co < args.min_count or co <= cn))):
            skipped["evidence"] += 1
            if args.verbose:
                print(f"evidence\t{old}\t{new}\t{'list' if in_list else 'corpus'} {co}/{cn}", file=sys.stderr)
            continue
        if new == old:
            continue
        found[old] = (new, "both" if both else "old", f"{'list ' + str(words[old]) if in_list else 'corpus'} "
                           f"-> {words[new] if new in words else 'new'}; corpus {co}/{cn}")

    # respell-accent: list words without accents, the corpus's accented form.
    by_fold = {}
    for k, (c, cased) in counts.items():
        by_fold.setdefault(fold(k), []).append((c, cased))
    for w in words:
        if w in found or w in missp or marks(w) or not accents:
            continue
        for suffix_re, accented_re in accents:
            # Any split into a stem that is a word and a suffix.
            if not any(w[:i] in words and suffix_re.fullmatch(w[i:]) for i in range(2, len(w) - 1)):
                continue
            forms = [(c, f) for c, f in by_fold.get(fold(w), ()) if f != w and accented_re.search(f)]
            if not forms:
                continue
            cn, new = max(forms)
            if w.islower():
                new = new.lower()   # Háblanos: seen at sentence starts only
            co = count(w)
            if w in keep:
                skipped["keep"] += 1
                break
            if cn < args.min_count or cn < args.ratio * co:
                skipped["evidence"] += 1
                if args.verbose:
                    print(f"evidence\t{w}\t{new}\tlist {co}/{cn}", file=sys.stderr)
                break
            found[w] = (new, "old", f"list {words[w]} -> {words[new] if new in words else 'new'}; "
                                    f"corpus {co}/{cn}")
            break

    out = sorted(found.items(), key=lambda kv: kv[0].lower())
    with open(args.out, "w", encoding="utf-8") as fh:
        # The header of the written file, split so that reuse doesn't take it as this file's.
        fh.write("# SPDX-" "FileCopyrightText: Mozilla Common Voice contributors, Tatoeba contributors\n"
                 "# SPDX-" "License-Identifier: LicenseRef-CC-BY-2.0-FR AND CC0-1.0\n"
                 "# Generated by tools/data/find-respellings.py from the `respell` rules of the .rules file,\n"
                 "# the AOSP list and word counts of the Common Voice (CC0) and Tatoeba (CC BY 2.0 FR)\n"
                 "# sentences; see THIRD-PARTY.md. Review changes.\n"
                 "# old\tcurrent\tkind (old: a misspelling now; both: both in use)\t"
                 "evidence (where the old form is from, list f; corpus count old/current)\n")
        for old, (new, kind, ev) in out:
            fh.write(f"{old}\t{new}\t{kind}\t{ev}\n")
    n_list = sum(1 for _, (_, _, ev) in out if ev.startswith("list"))
    n_both = sum(1 for _, (_, kind, _) in out if kind == "both")
    print(f"{args.lang}: {len(out)} respelled ({n_list} from the list, {len(out) - n_list} from the corpus; "
          f"{n_both} both in use); "
          "not: " + ", ".join(f"{k} {v}" for k, v in skipped.items()), file=sys.stderr)


if __name__ == "__main__":
    main()
