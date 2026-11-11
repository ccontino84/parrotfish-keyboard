#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The emoji the panel offers (layouts/parrotfish/emoji/emojidata.js), from Unicode's
emoji-test.txt (https://unicode.org/Public/emoji/latest/emoji-test.txt):
the fully-qualified emoji in Unicode's order, by group, without the
Component group (tones and hair styles on their own).

Variants are not in the grid but behind their emoji's long press
(`variants`): its skin tones and, for a person, the man's and the
woman's forms (with their tones). A variant is an emoji whose code
points, without skin tones and FE0F, are another emoji's; it has one tone
throughout (two people of the same tone), mixed tones are left out. The
gender forms are matched by name (person/man/woman, people/men/women and
a few pairs of their own).

Activities and Objects are one group, Activities & Objects (most
activities are things too: balls, games, crafts; they follow each other:
crafts, then clothing), so that the panel's tabs fit. A group's tab shows
its first emoji, or its ICONS entry.

MAX_VERSION leaves out emoji newer than that Emoji version (the system
font's coverage: Noto Emoji 17.0 on Sailfish OS 5.2).

usage: emoji-list.py EMOJI_TEST_TXT OUT.js [MAX_VERSION]
"""
import re
import sys

TONES = [chr(c) for c in range(0x1F3FB, 0x1F400)]   # light to dark
LINE = re.compile(r"^([0-9A-F ]+?)\s*; fully-qualified\s*# (\S+) E(\d+\.\d+) (.*)$")
MERGED = {"Activities": "Activities & Objects", "Objects": "Activities & Objects"}
# Tab icons other than the group's first emoji (🎃, 🏧, 🏁).
ICONS = {"Activities & Objects": "\u26bd",           # ⚽
         "Symbols": "#\ufe0f\u20e3",                 # #️⃣
         "Flags": "\U0001f6a9"}                       # 🚩
# Gender forms whose names don't follow the pattern. (Not Santa Claus and
# Mrs. Claus, nor prince and princess: characters of their own, common
# enough to stay in the grid.)
GENDER_PAIRS = {
    "older person": ("old man", "old woman"),
    "child": ("boy", "girl"),
}


def key(text):
    """An emoji without skin tones and FE0F: the same for all its variants."""
    return "".join(c for c in text if c not in TONES and c != "️")


def gender_forms(name, names):
    """The man's and the woman's forms of a person emoji's name, or None."""
    candidates = []
    if name in GENDER_PAIRS:
        candidates.append(GENDER_PAIRS[name])
    if name.startswith("person "):
        candidates.append(("man " + name[7:], "woman " + name[7:]))
    if name.startswith("person:"):
        candidates.append(("man" + name[6:], "woman" + name[6:]))
    if not name.startswith(("man", "woman")):
        candidates.append(("man " + name, "woman " + name))
    if "person" in name:
        candidates.append((name.replace("person", "man", 1), name.replace("person", "woman", 1)))
    if name.startswith("people "):
        candidates.append(("men " + name[7:], "women " + name[7:]))
    for man, woman in candidates:
        if man in names and woman in names:
            return man, woman
    return None


def main():
    source, out = sys.argv[1], sys.argv[2]
    max_version = float(sys.argv[3]) if len(sys.argv) > 3 else 99.0
    version = ""
    groups = []      # (name, [emoji]): emoji without tones
    names = {}       # emoji's name: the emoji
    base_of = {}     # key(): the emoji without tones
    tones = {}       # emoji: its tone variants, light to dark
    toned = []       # (emoji with tones, its tone)
    with open(source, encoding="utf-8") as f:
        for line in f:
            if line.startswith("# Version:"):
                version = line.split(":", 1)[1].strip()
            elif line.startswith("# group:"):
                name = line.split(":", 1)[1].strip()
                name = MERGED.get(name, name)
                if not groups or groups[-1][0] != name:
                    groups.append((name, []))
            else:
                m = LINE.match(line)
                if not m or not groups or groups[-1][0] == "Component" or float(m.group(3)) > max_version:
                    continue
                text = "".join(chr(int(c, 16)) for c in m.group(1).split())
                found = {c for c in text if c in TONES}
                if found:
                    if len(found) == 1:   # mixed tones: left out
                        toned.append((text, TONES.index(found.pop())))
                    continue
                names[m.group(4).strip()] = text
                base_of.setdefault(key(text), text)
                groups[-1][1].append(text)
    groups = [g for g in groups if g[0] != "Component"]
    for text, tone in toned:
        base = base_of.get(key(text))
        if base:
            tones.setdefault(base, [None] * 5)[tone] = text

    def row(emoji):
        t = tones.get(emoji)
        return [emoji] + t if t and all(t) else [emoji]

    variants = {}    # emoji: rows of variants (its own first)
    folded = set()   # man's and woman's forms, behind the person's
    for name, emoji in names.items():
        forms = gender_forms(name, names)
        if forms and emoji not in folded:
            man, woman = names[forms[0]], names[forms[1]]
            variants[emoji] = [row(emoji), row(man), row(woman)]
            folded.update((man, woman))
    for name, emoji in names.items():
        if emoji not in variants and emoji not in folded and len(row(emoji)) > 1:
            variants[emoji] = [row(emoji)]
    groups = [(n, [e for e in emojis if e not in folded]) for n, emojis in groups]

    with open(out, "w", encoding="utf-8") as f:
        # REUSE-IgnoreStart (the header of the file written, not this one's)
        f.write("// SPDX-FileCopyrightText: 2026 Unicode, Inc.\n")
        f.write("// SPDX-License-Identifier: Unicode-3.0\n")
        # REUSE-IgnoreEnd
        f.write("// Generated by tools/data/emoji-list.py from emoji-test.txt, Emoji %s\n" % version)
        f.write("// (up to Emoji %s). Each group's emoji are separated by spaces.\n" % max_version)
        f.write(".pragma library\n\n")
        f.write("var groups = [\n")
        for name, emojis in groups:
            f.write('    { "name": "%s", "icon": "%s", "emojis": "%s" },\n'
                    % (name, ICONS.get(name, emojis[0]), " ".join(emojis)))
        f.write("]\n\n")
        f.write("// An emoji's variants, in rows separated by \"|\" (the person's, the\n")
        f.write("// man's, the woman's), each its emoji then its skin tones (light to\n")
        f.write("// dark), separated by spaces.\n")
        f.write("var variants = {\n")
        for name, emojis in groups:
            for e in emojis:
                if e in variants:
                    f.write('    "%s": "%s",\n' % (e, "|".join(" ".join(r) for r in variants[e])))
        f.write("}\n")
    print("%d groups, %d emoji, %d with variants (%d with gender forms, %d folded)"
          % (len(groups), sum(len(g[1]) for g in groups), len(variants),
             sum(1 for v in variants.values() if len(v) == 3), len(folded)))
    for name, emojis in groups:
        print("  %s: %d" % (name, len(emojis)))
    # Left in the grid: men's and women's forms without a person's.
    left = [n for n, e in names.items() if e not in folded and re.match(r"(man|woman|men|women)\b", n)]
    print("unfolded man/woman forms:", len(left), left[:40])


if __name__ == "__main__":
    main()
