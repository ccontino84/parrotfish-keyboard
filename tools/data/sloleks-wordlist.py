#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The Slovenian wordlist (data/sloleks/sl_wordlist.combined.gz) from the
Sloleks 3.1 morphological lexicon (CC BY-SA 4.0, hdl.handle.net/11356/2080),
in the AOSP format that tools/data/build-dict.py reads.

Usage: sloleks-wordlist.py SLOLEKS.zip SCALE_WORDLIST.gz OUT.gz [--top N]

Each word form of the lexicon has its count in the Gigafida 2.0 corpus. Kept:
forms of letters only (with inner ' or -), as Sloleks spells them (names
capitalised), of common words and of names entered by hand; names Sloleks
added automatically were never checked (the, obama), so they are left out.
The N most frequent (default 250,000: the size of the other lists; the typo
benchmarks chose it over 150,000 and 400,000) get the frequencies of
SCALE_WORDLIST by rank (the Croatian AOSP list, of a similar size), so that
they sit on the same scale as the other languages' lists.

Reading the lexicon (13 GB of XML in the zip) takes several minutes.
"""

import argparse
import gzip
import re
import sys
import xml.etree.ElementTree as ET
import zipfile

WORD = re.compile(r"^[^\W\d_]+(?:['-][^\W\d_]+)*$")


def forms(path):
    """(form, Gigafida count, kept) for every orthographic form of the lexicon."""
    with zipfile.ZipFile(path) as z:
        for name in sorted(n for n in z.namelist() if n.endswith(".xml")):
            with z.open(name) as fh:
                for _, el in ET.iterparse(fh, events=("end",)):
                    if el.tag != "entry":
                        continue
                    head = el.find("head")
                    manual = (head.findtext("status") or "").strip() == "MANUAL"
                    proper = (head.findtext("grammar/category") or "").strip() == "noun" and any(
                        g.get("name") == "type" and (g.text or "").strip() == "proper"
                        for g in head.findall("grammar/grammarFeature"))
                    for o in el.iterfind("body/wordFormList/wordForm/formRepresentations/orthographyList/orthography"):
                        form = (o.findtext("form") or "").strip()
                        count = 0
                        for m in o.iterfind("measureList/measure"):
                            if m.get("type") == "frequency":
                                try:
                                    count = max(count, int(m.text))
                                except (TypeError, ValueError):
                                    pass
                        yield form, count, manual or not proper
                    el.clear()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sloleks")
    ap.add_argument("scale")
    ap.add_argument("out")
    ap.add_argument("--top", type=int, default=250000)
    args = ap.parse_args()
    counts = {}
    for form, count, kept in forms(args.sloleks):
        if kept and count > 0 and WORD.match(form):
            counts[form] = max(counts.get(form, 0), count)
    entries = sorted(counts.items(), key=lambda e: (-e[1], e[0]))[:args.top]
    with gzip.open(args.scale, "rt", encoding="utf-8") as fh:
        scale = sorted((int(m.group(1)) for m in re.finditer(r"\bf=(\d+)", fh.read())), reverse=True)
    with gzip.open(args.out, "wt", encoding="utf-8") as out:
        out.write("dictionary=main:sl,locale=sl,description=Slovenščina,date=0,version=1\n")
        for i, (word, _) in enumerate(entries):
            f = scale[min(i, len(scale) - 1)]
            out.write(f" word={word},f={f},flags=,originalFreq={f}\n")
    print(f"{args.out}: {len(entries)} words (of {len(counts)}); the last seen {entries[-1][1]} times",
          file=sys.stderr)


if __name__ == "__main__":
    main()
