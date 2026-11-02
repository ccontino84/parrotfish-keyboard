#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
"""Build a Parrotfish dictionary (.pfd) from an AOSP LatinIME wordlist.

Usage: build-dict.py LANG WORDLIST.combined.gz RULES_DIR OUT.pfd [--pairs PAIRS.tsv.gz]
                     [--letter-order N] [--letter-min-count N]

The output is a little-endian binary file that the engine memory-maps.
The layout is documented in engine/include/parrotfish/dictformat.h; keep
both in sync (FORMAT_VERSION).
"""

import argparse
import collections
import gzip
import math
import struct
import sys
import unicodedata

FORMAT_VERSION = 4
MAGIC = b"PFDICT\0\0"

# Attribute flags (dictformat.h: AttrFlag)
ATTR_OFFENSIVE = 1 << 0      # valid, never suggested unprompted
ATTR_MISSPELLING = 1 << 1    # not a word: corrected to `target`
ATTR_ABBREVIATION = 1 << 2
ATTR_NONWORD = 1 << 3        # informal (gonna, kinda): valid, ranked as is
ATTR_SHORTCUT = 1 << 4       # valid word with an extra suggestion `target`

# Word-pair rows (dictformat.h): previous word = start of a sentence / anything.
PAIR_START = 0
PAIR_ANY = 0xFFFFFFFF

# Frequency used for a misspelling whose target is several words (alot → a lot).
MULTIWORD_TARGET_FREQ = 100

# Rule flags (dictformat.h: RuleFlag)
RULE_FINAL = 1 << 0          # only at the end of the word

# AOSP frequencies: f=255 is probability 1, each step divides it by 1.15
# (dictionaries/sample.combined).
STEP = math.log(1.15)

# Optional sections after the fixed ones (dictformat.h: Section): tag, version.
SECTION_LETTERS = (b"LTRS", 1)
# The letter model (dictformat.h: LetterModel): symbols of 6 bits (0 none,
# 1 the word boundary), costs in steps of 1/16 nat, 255 for none.
LETTER_SYMBOLS = 64
LETTER_STEP = 1.0 / 16
LETTER_NONE = 255
LETTER_ORDER = 4        # 3 letters of context (5: no better, three times the size)
LETTER_MIN_COUNT = 2


def fold(word):
    """Lowercase, strip combining marks, ß→ss: the key for accent pairs."""
    s = unicodedata.normalize("NFD", word.lower())
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    return s.replace("ß", "ss")


# Letters with no Unicode decomposition that are typed as a plain letter
# when the keyboard lacks them (Danish/Norwegian ø, æ; French œ).
BASE_EXTRA = {"ø": "o", "æ": "a", "œ": "o"}


def base_char(c):
    """Lowercase base letter of one character, for accent-free matching."""
    if c.lower() in BASE_EXTRA:
        return BASE_EXTRA[c.lower()]
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


def apply_respellings(words, path):
    """Apply RULES_DIR/LANG.respellings (find-respellings.py): the current
    form gets at least the old form's frequency (added if missing); an old
    form of kind `old` becomes a misspelling of it (added if missing), one
    of kind `both` stays. Returns (added, marked)."""
    added = marked = 0
    for line in read_list(path):
        old, new, kind = line.split("\t")[:3]
        eo, en = words.get(old), words.get(new)
        if eo is None and en is None:
            sys.exit(f"{path}: neither {old!r} nor {new!r} is in the wordlist")
        if eo is not None and eo["attrs"] & ATTR_MISSPELLING:
            continue
        f = max(eo["f"] if eo else 0, en["f"] if en else 0)
        if en is None:
            words[new] = {"f": f, "attrs": 0, "target": None}
            added += 1
        elif not en["attrs"] & ATTR_MISSPELLING:
            en["f"] = f
        else:
            sys.exit(f"{path}: {new!r} is a misspelling in the wordlist")
        if kind == "old":
            words[old] = {"f": f, "attrs": ATTR_MISSPELLING, "target": new}
            marked += 1
    return added, marked


def apply_boosts(words, path):
    """Set the everyday frequencies of RULES_DIR/LANG.boost (find-boosts.py),
    raised or lowered. Returns (raised, lowered)."""
    raised = lowered = 0
    for line in read_list(path):
        w, f = line.split("\t")[:2]
        e = words.get(w)
        if e is not None and int(f) != e["f"] and not e["attrs"] & ATTR_MISSPELLING:
            raised += int(f) > e["f"]
            lowered += int(f) < e["f"]
            e["f"] = int(f)
    return raised, lowered


def mark_misspellings(words, path):
    """Apply RULES_DIR/LANG.misspellings (from find-accent-misspellings.py)
    or LANG.manual-misspellings (hand-written)."""
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
        if parts[0] in ("paradigm", "paradigm-plain", "only-marks", "no-misspellings",
                        "capitalised-nouns", "min-freq", "one-source", "suffix", "compound",
                        "final-apostrophe", "respell", "respell-accent"):
            continue   # settings for the generators (min-freq, suffix, compound: also main())
        flags = RULE_FINAL if "final" in parts[2:] else 0
        if not 1 <= len(parts[0]) <= 3 or len(parts[1]) != 1:
            sys.exit(f"{path}: bad rule {line!r}")
        rules.append((parts[0], parts[1], flags))
    return rules


def word_parts(path):
    """The `suffix` and `compound` lines of a rules file, for the engine
    (dictformat.h: Header::wordParts), or "" if none."""
    return "\n".join(" ".join(line.split()) for line in read_list(path)
                     if line.split()[0] in ("suffix", "compound"))


def min_freq(path):
    """The `min-freq N` setting of a rules file, or 0."""
    for line in read_list(path):
        parts = line.split()
        if parts[0] == "min-freq":
            return int(parts[1])
    return 0


def read_pairs(path, words, order):
    """Word pairs from count-pairs.py, resolved to trie nodes.

    Returns {prev node: [(next node, f)]}, f on the 0..255 scale of the
    conditional probability P(next | prev).
    """
    by_lower = {}
    for n in order:
        if not n.terminal:
            continue
        e = words[n.word]
        if e["attrs"] & (ATTR_MISSPELLING | ATTR_OFFENSIVE):
            continue
        k = n.word.lower()
        best = by_lower.get(k)
        # Prefer the lowercase spelling, then the most frequent one.
        rank = (n.word == k, e["f"])
        if best is None or rank > best[0]:
            by_lower[k] = (rank, n.index)
    exact = {n.word: n.index for n in order
             if n.terminal and not words[n.word]["attrs"] & (ATTR_MISSPELLING | ATTR_OFFENSIVE)}
    rows = collections.defaultdict(list)
    totals = {}
    with gzip.open(path, "rt", encoding="utf-8") as fh:
        for line in fh:
            prev, nxt, count = line.rstrip("\n").split("\t")
            if prev == "<s>":
                key = PAIR_START
            elif prev == "<any>":
                key = PAIR_ANY
            elif prev in by_lower:
                key = by_lower[prev][1]
            else:
                continue
            if nxt == "*":
                totals[key] = int(count)
                continue
            # The exact spelling (German nouns), else any case.
            node = exact.get(nxt) or (by_lower[nxt.lower()][1] if nxt.lower() in by_lower else None)
            if node is not None and key in totals:
                p = int(count) / totals[key]
                f = max(1, min(255, round(255 + math.log(p) / STEP)))
                rows[key].append((node, f))
    return rows


def letter_model(words, order, min_count):
    """The letter-pattern model of the language (dictformat.h: LetterModel).

    A character n-gram over the word list's distinct words, lowercase and
    without accents (as the engine folds typed letters), each word once: an
    unknown word is a rare one, so the shape of the list's words matters,
    not how often they are used. Interpolated Witten-Bell estimates, stored
    in back-off form (ARPA): for a kept n-gram -log P(c | h), for a context
    -log of its back-off weight. N-grams of the full order seen fewer than
    `min_count` times are dropped and their mass moved to the back-off (a
    quarter smaller; dropping shorter ones too changes more names).

    Returns (section bytes, number of n-grams, mean cost per letter).
    """
    train = set()
    for w, e in words.items():
        if e["attrs"] & ATTR_MISSPELLING or not all(c.isalpha() or c in "'-" for c in w):
            continue
        train.add("".join(base_char(c) for c in w.lower()))
    train = sorted(train)   # the same file from the same list
    freq = collections.Counter(c for w in train for c in w)
    if len(freq) > LETTER_SYMBOLS - 2:
        sys.exit("too many letters for the letter model")
    # Symbol 0: none (key padding), 1: the word boundary, then by frequency.
    symbols = [c for c, _ in freq.most_common()]
    sym = {c: i + 2 for i, c in enumerate(symbols)}

    # counts[k][(h, c)]: c after the k-1 symbols h (tuples of symbol ids).
    counts = [None] + [collections.Counter() for _ in range(order)]
    for w in train:
        s = (1,) + tuple(sym[c] for c in w) + (1,)
        for i in range(1, len(s)):
            for k in range(1, min(order, i + 1) + 1):
                counts[k][(s[i - k + 1:i], s[i])] += 1
    # Per context: total count and number of distinct followers.
    totals = [None] + [collections.defaultdict(lambda: [0, 0]) for _ in range(order)]
    for k in range(1, order + 1):
        for (h, c), n in counts[k].items():
            t = totals[k][h]
            t[0] += n
            t[1] += 1

    # Interpolated Witten-Bell: P(c | h) = (n(h c) + T(h) P(c | h')) / (C(h) + T(h)),
    # h' = h without its first symbol; below the unigram, uniform.
    uniform = 1.0 / (len(symbols) + 1)
    prob = [None] + [{} for _ in range(order)]
    for k in range(1, order + 1):
        for (h, c), n in counts[k].items():
            if k == order and n < min_count:
                continue
            lower = prob[k - 1][(h[1:], c)] if k > 1 else uniform
            total, types = totals[k][h]
            prob[k][(h, c)] = (n + types * lower) / (total + types)

    def backed_off(h, c):
        """P(c | h) of the stored model."""
        weight = 1.0
        while True:
            p = prob[len(h) + 1].get((h, c))
            if p is not None:
                return weight * p
            if not h:
                return weight * uniform
            weight *= bow.get(h, 1.0)
            h = h[1:]

    # Back-off weights, shorter contexts first: the mass the kept n-grams
    # leave, spread like the shorter context's estimates.
    bow = {}
    for k in range(2, order + 1):
        kept = collections.defaultdict(lambda: [0.0, 0.0])
        for (h, c), p in prob[k].items():
            kept[h][0] += p
            kept[h][1] += backed_off(h[1:], c)
        for h, (p, lower) in kept.items():
            bow[h] = max(1e-9, 1.0 - p) / max(1e-9, 1.0 - lower)

    def quant(cost):
        return max(0, min(254, round(cost / LETTER_STEP)))

    # One sorted key per n-gram or context: symbol ids, 6 bits each, the
    # predicted symbol lowest.
    def key(seq):
        k = 0
        for s in seq:
            k = k << 6 | s
        return k
    entries = {}
    for k in range(1, order + 1):
        for (h, c), p in prob[k].items():
            entries[key(h + (c,))] = [quant(-math.log(p)), 0]
    for h, b in bow.items():
        e = entries.setdefault(key(h), [LETTER_NONE, 0])   # a context that is no kept n-gram
        e[1] = quant(-math.log(b))
    keys = sorted(entries)

    # Mean cost per letter of the list's words (end of word included).
    letters = cost = 0
    for w in train:
        s = (1,) + tuple(sym[c] for c in w) + (1,)
        for i in range(1, len(s)):
            cost -= math.log(backed_off(s[max(0, i - order + 1):i], s[i]))
        letters += len(w)
    mean = cost / letters

    head_fmt = "<IIIIIIff"
    off = struct.calcsize(head_fmt)
    sym_off = off
    off += 4 * len(symbols)
    off = (off + 7) & ~7
    keys_off = off
    off += 4 * len(keys)
    values_off = off
    body = struct.pack(head_fmt, order, len(symbols) + 2, sym_off, len(keys), keys_off, values_off,
                       LETTER_STEP, mean)
    body += b"".join(struct.pack("<I", ord(c)) for c in symbols)
    body += b"\0" * (keys_off - len(body))
    body += b"".join(struct.pack("<I", k) for k in keys)
    body += b"".join(struct.pack("<BB", *entries[k]) for k in keys)
    return bytes(body), len(keys), mean


class Node:
    __slots__ = ("children", "freq", "terminal", "best", "height", "index", "word")

    def __init__(self):
        self.children = {}
        self.freq = 0
        self.terminal = False
        self.best = 0
        self.height = 0
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

    # Best frequency and height (most letters below) of each subtree.
    def best(n):
        n.best = n.freq if n.terminal else 0
        for ch in n.children.values():
            n.best = max(n.best, best(ch))
            n.height = max(n.height, ch.height + 1)
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
    ap.add_argument("--pairs")
    ap.add_argument("--letter-order", type=int, default=LETTER_ORDER)
    ap.add_argument("--letter-min-count", type=int, default=LETTER_MIN_COUNT)
    args = ap.parse_args()
    if not 1 <= args.letter_order <= 5:
        sys.exit("--letter-order: 1 to 5")

    words = parse_aosp(args.wordlist)
    # `min-freq N` in LANG.rules drops the rare entries of a list padded
    # with generated forms (ro); the supplement adds back those in use.
    floor = min_freq(f"{args.rules_dir}/{args.lang}.rules")
    dropped = 0
    if floor:
        for w in [w for w, e in words.items() if e["f"] < floor and not e["attrs"] & ATTR_MISSPELLING]:
            del words[w]
            dropped += 1
    supplemented = add_supplement(words, f"{args.rules_dir}/{args.lang}.supplement")
    forms = add_supplement(words, f"{args.rules_dir}/{args.lang}.forms")
    respelled_new, respelled = apply_respellings(words, f"{args.rules_dir}/{args.lang}.respellings")
    marked = mark_misspellings(words, f"{args.rules_dir}/{args.lang}.misspellings")
    marked += mark_misspellings(words, f"{args.rules_dir}/{args.lang}.manual-misspellings")
    raised, lowered = apply_boosts(words, f"{args.rules_dir}/{args.lang}.boost")
    rules = read_rules(f"{args.rules_dir}/{args.lang}.rules")

    root, order = build_trie(words)

    alphabet = sorted({c for w in words for c in w})
    if len(alphabet) > 0x100:
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
    string_offsets = {}
    def word_string(w):
        if w not in string_offsets:
            string_offsets[w] = add_string(w)
        return string_offsets[w]

    attrs = []
    for n in order:
        if n.terminal:
            e = words[n.word]
            if e["attrs"]:
                target = add_string(e["target"]) if e["target"] else 0xFFFFFFFF
                attrs.append((n.index, e["attrs"], target))

    parts = word_parts(f"{args.rules_dir}/{args.lang}.rules")
    parts_ref = add_string(parts) + 1 if parts else 0

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
        struct.pack_into("<IBBBB", nodes, n.index * 8, link,
                         char_of.get(n.index, 0), min(n.height, 255), n.best, n.freq)

    alpha = bytearray()
    for c in alphabet:
        alpha += struct.pack("<III", ord(c), ord(c.lower()) if len(c.lower()) == 1 else ord(c),
                             ord(base_char(c)))

    attr_bytes = b"".join(struct.pack("<III", *a) for a in attrs)

    rule_bytes = bytearray()
    for inp, out, flags in rules:
        cps = [ord(c) for c in inp] + [0] * (3 - len(inp))
        rule_bytes += struct.pack("<IIIII", *cps, ord(out), flags)

    pair_rows = read_pairs(args.pairs, words, order) if args.pairs else {}
    row_bytes = bytearray()
    entry_bytes = bytearray()
    entries = 0
    for key in sorted(pair_rows):
        nexts = sorted(pair_rows[key], key=lambda x: -x[1])
        row_bytes += struct.pack("<IIHH", key, entries, len(nexts), 0)
        for node, f in nexts:
            entry_bytes += struct.pack("<IB3x", word_string(order[node].word), f)
            entries += 1

    letters, letter_ngrams, letter_mean = letter_model(words, args.letter_order, args.letter_min_count)
    optional = [(SECTION_LETTERS, letters)]

    lang = args.lang.encode()[:7].ljust(8, b"\0")
    header_fmt = "<8sII8sIfIIIIIIIIIIIIIIIIII"
    header_size = struct.calcsize(header_fmt)
    off = header_size
    sections = []
    blobs = [nodes, alpha, attr_bytes, rule_bytes, strings, row_bytes, entry_bytes] + [b for _, b in optional]
    for blob in blobs:
        off = (off + 7) & ~7
        sections.append(off)
        off += len(blob)
    # The table of optional sections, last.
    table_off = (off + 7) & ~7
    table = b"".join(struct.pack("<4sIII", tag, version, start, len(blob))
                     for ((tag, version), blob), start in zip(optional, sections[7:]))
    blobs.append(table)
    sections.append(table_off)

    header = struct.pack(header_fmt, MAGIC, FORMAT_VERSION, header_size, lang,
                         sum(1 for e in words.values() if not e["attrs"] & ATTR_MISSPELLING),
                         log_norm,
                         len(order), sections[0],
                         len(alphabet), sections[1],
                         len(attrs), sections[2],
                         len(rules), sections[3],
                         len(strings), sections[4],
                         len(pair_rows), sections[5],
                         entries, sections[6],
                         parts_ref, len(optional), table_off, 0)
    out = bytearray(header)
    for blob, start in zip(blobs, sections):
        out += b"\0" * (start - len(out))
        out += blob
    with open(args.out, "wb") as fh:
        fh.write(out)

    print(f"{args.lang}: {len(words)} entries ({dropped} below min-freq dropped, {supplemented} supplement, {forms} verb forms, "
          f"{raised} raised, {lowered} lowered), "
          f"{len(order)} nodes, "
          f"{len(alphabet)} chars, {len(attrs)} attrs ({marked} accent misspellings, "
          f"{respelled} old spellings, {respelled_new} current forms added), "
          f"{len(rules)} rules, {len(pair_rows)} pair rows ({entries} next words), "
          f"letter model {letter_ngrams} n-grams ({len(letters) / 1e6:.2f} MB, {letter_mean:.2f} nats a letter), "
          f"{len(out) / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
