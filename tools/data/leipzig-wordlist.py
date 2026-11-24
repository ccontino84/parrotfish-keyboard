#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""A wordlist (data/leipzig/LANG_wordlist.combined.gz) from Leipzig Corpora
Collection downloads (CC BY, wortschatz-leipzig.de), for languages AOSP has
no list for, in the AOSP format that tools/data/build-dict.py reads.

Usage: leipzig-wordlist.py LANG SCALE_WORDLIST.gz OUT.gz CORPUS.tar.gz...
                           [--top N] [--min-count N] [--acronym-min N]
                           [--foreign LIST.gz] [--foreign-ratio R]

LANG is uk or bg. Each CORPUS is a Leipzig archive (xxx_news_2024_1M.tar.gz);
the words of its *-sentences.txt are counted, a sentence repeated across
archives once. Words are letters with inner ' or - (’ ʼ ‘ ` count as ':
м'ясо); a word with a letter outside the language's alphabet (Latin, ы in
Ukrainian) or a digit is left out. Case:
- a word capitalised where a sentence or quote can't start, at least ten
  times as often as it is written in lowercase, is a name and keeps its
  capital (Київ); otherwise it is lowercase (віра, not the name Віра);
- a word mostly in capitals is an acronym, kept (flagged abbreviation, as
  AOSP does) only if seen --acronym-min times (default 1,000) or more;
- one-letter words only from the language's list (no initials, no р. for
  рік).
With --foreign (the Russian AOSP list), a word of that list much rarer in
the corpus than in the list (its rank more than R times lower, default 300)
is taken for a quote in that language and left out (что, человек), unless
it is among the 5,000 commonest or in the language's not_foreign set.
The N most frequent words (default 250,000, as the other lists; fewer when
words seen as often as the first one left out must go too) seen --min-count
times or more (default 5: the tail below has more typos) get the
frequencies of SCALE_WORDLIST by rank (the Russian AOSP list), so that they
sit on the same scale as the other languages' lists.
The same inputs give the same output, byte for byte.

Counting 5M sentences takes about three minutes.
"""

import argparse
import collections
import gzip
import hashlib
import io
import re
import sys
import tarfile
import unicodedata

LANGS = {
    "uk": {"description": "Українська",
           "letters": "абвгґдеєжзиіїйклмнопрстуфхцчшщьюя'-",
           "single": "абвєжзйіоуя",
           # Taken for Russian by --foreign, but Ukrainian too (checked by hand).
           "not_foreign": {"он", "но", "других", "смерти", "умер"}},
    "bg": {"description": "Български",
           "letters": "абвгдежзийклмнопрстуфхцчшщъьюяѝ-",
           "single": "авеиосуяѝ",
           "not_foreign": set()},
}

TOKEN = re.compile(r"[^\W\d_]+(?:['-][^\W\d_]+)*")
# A capital after these (or at the start) may just start a sentence or quote.
OPENER = re.compile(r"[.!?:;«\"“„…—–-]")
# The commonest words are never taken for foreign ones (по, так in Ukrainian).
FOREIGN_FLOOR = 5000
SUBST = str.maketrans({"’": "'", "ʼ": "'", "‘": "'", "`": "'", "‐": "-", "‑": "-"})
# Roman numerals written with Cyrillic letters (ХХІ століття).
ROMAN = re.compile(r"^[ІХ]+$")


def sentences(paths):
    """Sentences of the archives, NFC, without stress marks; repeats skipped."""
    seen = set()
    for path in paths:
        with tarfile.open(path, "r:gz") as tar:
            for m in tar:
                if not (m.isfile() and m.name.endswith("-sentences.txt")):
                    continue
                for raw in io.TextIOWrapper(tar.extractfile(m), encoding="utf-8", errors="replace"):
                    s = raw.rstrip("\n").split("\t", 1)[-1]
                    s = unicodedata.normalize("NFC", s.translate(SUBST))
                    if not s.isascii():
                        s = "".join(c for c in s if unicodedata.category(c) != "Mn")
                    h = hashlib.blake2b(s.encode(), digest_size=8).digest()
                    if h not in seen:
                        seen.add(h)
                        yield s


def count(paths):
    """{surface form: [count, count where a capital means a name]}."""
    counts = collections.defaultdict(lambda: [0, 0])
    for s in sentences(paths):
        end = 0
        for m in TOKEN.finditer(s):
            c = counts[m.group()]
            c[0] += 1
            if end and not OPENER.search(s, end, m.start()):
                c[1] += 1
            end = m.end()
    return counts


def choose(counts, lang, acronym_min):
    """{word: (count, acronym)}, one spelling per lowercase form."""
    letters, single = set(LANGS[lang]["letters"]), LANGS[lang]["single"]
    groups = collections.defaultdict(list)
    for form, (n, inner) in counts.items():
        low = form.lower()
        if set(low) <= letters and (len(low) > 1 or low in single):
            groups[low].append((form, n, inner))
    out = {}
    for low, forms in groups.items():
        total = sum(n for _, n, _ in forms)
        lower = sum(n for f, n, _ in forms if f[0].islower())
        caps = sum(n for f, n, _ in forms if len(f) > 1 and f.isupper())
        cap = [(i, f) for f, _, i in forms if f[0].isupper() and not (len(f) > 1 and f.isupper())]
        if caps > total - caps:
            if total >= acronym_min and not ROMAN.match(low.upper()):
                out[low.upper()] = (total, True)
        elif cap and max(cap)[0] > 0 and sum(i for i, _ in cap) >= 10 * lower:
            out[max(cap)[1]] = (total, False)
        else:
            out[low] = (total, False)
    return out


def ranks(path):
    """{lowercase word: rank} of an AOSP list, by frequency."""
    with gzip.open(path, "rt", encoding="utf-8") as fh:
        found = re.findall(r"^ word=([^,]+),f=(\d+)", fh.read(), re.M)
    found.sort(key=lambda e: -int(e[1]))
    out = {}
    for i, (w, _) in enumerate(found):
        out.setdefault(w.lower(), i + 1)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang", choices=sorted(LANGS))
    ap.add_argument("scale")
    ap.add_argument("out")
    ap.add_argument("corpora", nargs="+")
    ap.add_argument("--top", type=int, default=250000)
    ap.add_argument("--min-count", type=int, default=5)
    ap.add_argument("--acronym-min", type=int, default=1000)
    ap.add_argument("--foreign")
    ap.add_argument("--foreign-ratio", type=float, default=300)
    args = ap.parse_args()
    words = choose(count(args.corpora), args.lang, args.acronym_min)
    entries = sorted(((w, n, a) for w, (n, a) in words.items() if n >= args.min_count),
                     key=lambda e: (-e[1], e[0]))
    dropped = 0
    if args.foreign:
        foreign = ranks(args.foreign)
        kept = []
        for e in entries:
            r = foreign.get(e[0].lower())
            if (r and e[0] not in LANGS[args.lang]["not_foreign"]
                    and len(kept) + 1 > max(args.foreign_ratio * r, FOREIGN_FLOOR)):
                dropped += 1
            else:
                kept.append(e)
        entries = kept
    if len(entries) > args.top:
        # Words seen as often as the first one left out go too: no cut by spelling.
        last = entries[args.top][1]
        entries = [e for e in entries[:args.top] if e[1] > last]
    with gzip.open(args.scale, "rt", encoding="utf-8") as fh:
        # f=0 marks profanity (build-dict.py), not a frequency.
        scale = sorted((f for f in map(int, re.findall(r"\bf=(\d+)", fh.read())) if f), reverse=True)
    lines = [f"dictionary=main:{args.lang},locale={args.lang},"
             f"description={LANGS[args.lang]['description']},date=0,version=1\n"]
    for i, (word, _, acronym) in enumerate(entries):
        f = scale[min(i, len(scale) - 1)]
        flags = "abbreviation" if acronym else ""
        lines.append(f" word={word},f={f},flags={flags},originalFreq={f}\n")
    with open(args.out, "wb") as raw, gzip.GzipFile(fileobj=raw, mode="wb", filename="", mtime=0) as out:
        out.write("".join(lines).encode("utf-8"))
    print(f"{args.out}: {len(entries)} words (of {len(words)}; {dropped} foreign dropped); "
          f"the last seen {entries[-1][1]} times", file=sys.stderr)


if __name__ == "__main__":
    main()
