#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Build a Parrotfish dictionary (.pfd) from an AOSP LatinIME wordlist.

Usage: build-dict.py LANG WORDLIST.combined.gz RULES_DIR OUT.pfd

The output is a little-endian binary file that the engine memory-maps.
The layout is documented in engine/include/parrotfish/dictformat.h; keep
both in sync (FORMAT_VERSION).
"""

import argparse
import gzip
import math
import struct
import sys
import unicodedata

FORMAT_VERSION = 1
MAGIC = b"PFDICT\0\0"

# Attribute flags (dictformat.h: AttrFlag)
ATTR_OFFENSIVE = 1 << 0      # valid, never suggested unprompted
ATTR_MISSPELLING = 1 << 1    # not a word: corrected to `target`
ATTR_ABBREVIATION = 1 << 2
ATTR_NONWORD = 1 << 3        # informal (gonna, kinda): valid, ranked as is
ATTR_SHORTCUT = 1 << 4       # valid word with an extra suggestion `target`

# Frequency used for a misspelling whose target is several words (alot → a lot).
MULTIWORD_TARGET_FREQ = 100

# Rule flags (dictformat.h: RuleFlag)
RULE_FINAL = 1 << 0          # only at the end of the word

# AOSP frequencies: f=255 is probability 1, each step divides it by 1.15
# (dictionaries/sample.combined).
STEP = math.log(1.15)


def fold(word):
    """Lowercase, strip combining marks, ß→ss: the key for accent pairs."""
    s = unicodedata.normalize("NFD", word.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace("ß", "ss")


def base_char(c):
    """Lowercase base letter of one character, for accent-free matching."""
    d = unicodedata.normalize("NFD", c.lower())
    b = "".join(x for x in d if unicodedata.category(x) != "Mn")
    return b if len(b) == 1 else c.lower()


def parse_aosp(path):
    """Return {word: entry}; entry = dict(f, attrs, target)."""
    words = {}
    last = None
    with gzip.open(path, "rt", encoding="utf-8") as fh:
        for line in fh:
            if line.startswith(" word="):
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                w = fields["word"]
                attrs = 0
                flags = fields.get("flags", "")
                if "offensive" in flags or fields.get("possibly_offensive") == "true":
                    attrs |= ATTR_OFFENSIVE
                if "abbreviation" in flags:
                    attrs |= ATTR_ABBREVIATION
                if "nonword" in flags:
                    attrs |= ATTR_NONWORD
                if fields.get("not_a_word") == "true":
                    attrs |= ATTR_MISSPELLING
                f = int(fields["f"])
                if f == 0 and not attrs & ATTR_MISSPELLING:
                    attrs |= ATTR_OFFENSIVE   # f=0 marks profanity
                # Duplicates differ only in flags; keep the higher frequency.
                if w in words and words[w]["f"] >= f:
                    last = words[w]
                    continue
                last = words[w] = {"f": f, "attrs": attrs, "target": None}
            elif line.startswith("  shortcut=") and last is not None:
                fields = dict(kv.split("=", 1) for kv in line.strip().split(","))
                if last["target"] is None:
                    last["target"] = fields["shortcut"]
                    if not last["attrs"] & ATTR_MISSPELLING:
                        last["attrs"] |= ATTR_SHORTCUT
    return words


def read_list(path):
    try:
        with open(path, encoding="utf-8") as fh:
            return [l.split("#", 1)[0].strip() for l in fh
                    if l.split("#", 1)[0].strip()]
    except FileNotFoundError:
        return []


def add_supplement(words, path):
    """Add RULES_DIR/LANG.supplement (from find-supplement.py)."""
    added = 0
    for line in read_list(path):
        w, f = line.split("\t")[:2]
        if w not in words:
            words[w] = {"f": int(f), "attrs": 0, "target": None}
            added += 1
    return added


def mark_misspellings(words, path):
    """Apply RULES_DIR/LANG.misspellings (from find-accent-misspellings.py)."""
    marked = 0
    for line in read_list(path):
        plain, target = line.split("\t")[:2]
        e = words.get(plain)
        if e is None or target not in words:
            sys.exit(f"{path}: {plain!r} or {target!r} is not in the wordlist")
        e["attrs"] |= ATTR_MISSPELLING
        e["attrs"] &= ~ATTR_SHORTCUT
        e["target"] = target
        marked += 1
    return marked


def read_rules(path):
    """Rules file: `INPUT OUTPUT [final]` per line, e.g. `ae ä` or `e' è final`."""
    rules = []
    for line in read_list(path):
        parts = line.split()
        if parts[0] in ("paradigm", "only-marks", "capitalised-nouns"):
            continue   # used by find-accent-misspellings.py
        flags = RULE_FINAL if "final" in parts[2:] else 0
        if not 1 <= len(parts[0]) <= 3 or len(parts[1]) != 1:
            sys.exit(f"{path}: bad rule {line!r}")
        rules.append((parts[0], parts[1], flags))
    return rules


class Node:
    __slots__ = ("children", "freq", "terminal", "best", "index", "word")

    def __init__(self):
        self.children = {}
        self.freq = 0
        self.terminal = False
        self.best = 0
        self.index = 0
        self.word = None


def build_trie(words):
    root = Node()
    for w, e in words.items():
        n = root
        for c in w:
            n = n.children.setdefault(c, Node())
        n.terminal = True
        n.word = w
        n.freq = e["f"]
        if e["attrs"] & ATTR_MISSPELLING:
            # The search ranks a misspelling by the word it is corrected to.
            t = words.get(e["target"])
            n.freq = t["f"] if t else MULTIWORD_TARGET_FREQ

    def best(n):
        n.best = n.freq if n.terminal else 0
        for ch in n.children.values():
            n.best = max(n.best, best(ch))
        return n.best

    sys.setrecursionlimit(10000)
    best(root)

    # Breadth-first so each node's children are contiguous; siblings sorted
    # by their best frequency so the search finds good words first.
    order = [root]
    i = 0
    while i < len(order):
        n = order[i]
        kids = sorted(n.children.items(), key=lambda kv: (-kv[1].best, kv[0]))
        n.children = dict(kids)
        order.extend(k for _, k in kids)
        i += 1
    for idx, n in enumerate(order):
        n.index = idx
    return root, order


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lang")
    ap.add_argument("wordlist")
    ap.add_argument("rules_dir")
    ap.add_argument("out")
    args = ap.parse_args()

    words = parse_aosp(args.wordlist)
    supplemented = add_supplement(words, f"{args.rules_dir}/{args.lang}.supplement")
    marked = mark_misspellings(words, f"{args.rules_dir}/{args.lang}.misspellings")
    rules = read_rules(f"{args.rules_dir}/{args.lang}.rules")

    root, order = build_trie(words)

    alphabet = sorted({c for w in words for c in w})
    if len(alphabet) > 0xFFFF:
        sys.exit("alphabet too large")
    cindex = {c: i for i, c in enumerate(alphabet)}

    # Probability mass of real words, so languages are comparable.
    mass = sum(math.exp((e["f"] - 255) * STEP) for e in words.values()
               if not e["attrs"] & ATTR_MISSPELLING)
    log_norm = math.log(mass)

    strings = bytearray()
    def add_string(s):
        off = len(strings)
        strings.extend(s.encode("utf-8") + b"\0")
        return off

    attrs = []
    for n in order:
        if n.terminal:
            e = words[n.word]
            if e["attrs"]:
                target = add_string(e["target"]) if e["target"] else 0xFFFFFFFF
                attrs.append((n.index, e["attrs"], target))

    if len(order) >= 1 << 30:
        sys.exit("too many nodes")
    nodes = bytearray(8 * len(order))
    # The last-sibling bit belongs to the child, so collect it per parent.
    last_sibling = set()
    for n in order:
        kids = list(n.children.values())
        if kids:
            last_sibling.add(kids[-1].index)
    last_sibling.add(0)  # root
    char_of = {}
    for n in order:
        for c, k in n.children.items():
            char_of[k.index] = cindex[c]
    for n in order:
        kids = list(n.children.values())
        first = kids[0].index if kids else 0
        link = first | (1 << 30 if n.terminal else 0) | (1 << 31 if n.index in last_sibling else 0)
        struct.pack_into("<IHBB", nodes, n.index * 8, link,
                         char_of.get(n.index, 0), n.best, n.freq)

    alpha = bytearray()
    for c in alphabet:
        alpha += struct.pack("<III", ord(c), ord(c.lower()) if len(c.lower()) == 1 else ord(c),
                             ord(base_char(c)))

    attr_bytes = b"".join(struct.pack("<III", *a) for a in attrs)

    rule_bytes = bytearray()
    for inp, out, flags in rules:
        cps = [ord(c) for c in inp] + [0] * (3 - len(inp))
        rule_bytes += struct.pack("<IIIII", *cps, ord(out), flags)

    lang = args.lang.encode()[:7].ljust(8, b"\0")
    header_fmt = "<8sII8sIfIIIIIIIIIIII"
    header_size = struct.calcsize(header_fmt)
    off = header_size
    sections = []
    for blob in (nodes, alpha, attr_bytes, rule_bytes, strings):
        off = (off + 7) & ~7
        sections.append(off)
        off += len(blob)

    header = struct.pack(header_fmt, MAGIC, FORMAT_VERSION, header_size, lang,
                         sum(1 for e in words.values() if not e["attrs"] & ATTR_MISSPELLING),
                         log_norm,
                         len(order), sections[0],
                         len(alphabet), sections[1],
                         len(attrs), sections[2],
                         len(rules), sections[3],
                         len(strings), sections[4],
                         0, 0)
    out = bytearray(header)
    for blob, start in zip((nodes, alpha, attr_bytes, rule_bytes, strings), sections):
        out += b"\0" * (start - len(out))
        out += blob
    with open(args.out, "wb") as fh:
        fh.write(out)

    print(f"{args.lang}: {len(words)} entries ({supplemented} supplement), {len(order)} nodes, "
          f"{len(alphabet)} chars, {len(attrs)} attrs ({marked} accent misspellings), "
          f"{len(rules)} rules, {len(out) / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
