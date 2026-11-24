#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# The RPM's file lists: every dictionary goes to a language package by
# script: parrotfish-keyboard-languages-latin (or, for the Latin languages
# marked experimental in languages.json,
# parrotfish-keyboard-languages-latin-experimental),
# parrotfish-keyboard-languages-cyrillic and
# parrotfish-keyboard-languages-greek; the keyboard's package keeps none.
#   split-languages.py INSTALLED_DATA_DIR DATA_DIR
# writes latin.files, latin-experimental.files, cyrillic.files, greek.files
# and main-excludes.files in the current directory. It lives outside rpm/
# because SailfishOS:Chum's build service leaves rpm/ out of the sources
# (DEVELOPMENT.md).
import json
import sys

installed, path = sys.argv[1], sys.argv[2]
with open(installed + "/languages.json", encoding="utf-8") as f:
    langs = json.load(f)
script = lambda l: l.get("script", "Latn")
packages = {
    "latin": sorted(c for c, l in langs.items() if script(l) == "Latn" and not l["experimental"]),
    "latin-experimental": sorted(c for c, l in langs.items() if script(l) == "Latn" and l["experimental"]),
    "cyrillic": sorted(c for c, l in langs.items() if script(l) == "Cyrl"),
    "greek": sorted(c for c, l in langs.items() if script(l) == "Grek"),
}
if sum(len(codes) for codes in packages.values()) != len(langs):
    sys.exit("split-languages.py: a language of another script")
for name, codes in packages.items():
    if not codes:
        sys.exit(f"split-languages.py: parrotfish-keyboard-languages-{name} would be empty")
    with open(f"{name}.files", "w") as f:
        f.writelines(f"{path}/{c}.pfd\n" for c in codes)
    print(f"parrotfish-keyboard-languages-{name}:", " ".join(codes))
with open("main-excludes.files", "w") as f:
    f.writelines(f"%exclude {path}/{c}.pfd\n" for c in sorted(langs))
