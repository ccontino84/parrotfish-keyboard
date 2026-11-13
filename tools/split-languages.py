#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# The RPM's file lists: every dictionary goes to a language package,
# parrotfish-keyboard-languages-latin or, for the languages marked
# experimental in languages.json,
# parrotfish-keyboard-languages-latin-experimental; the keyboard's package
# keeps none. (Every language is in the Latin script so far.)
#   split-languages.py INSTALLED_DATA_DIR DATA_DIR
# writes latin.files, latin-experimental.files and main-excludes.files in
# the current directory. It lives outside rpm/ because SailfishOS:Chum's
# build service leaves rpm/ out of the sources (DEVELOPMENT.md).
import json
import sys

installed, path = sys.argv[1], sys.argv[2]
with open(installed + "/languages.json", encoding="utf-8") as f:
    langs = json.load(f)
experimental = sorted(c for c, l in langs.items() if l["experimental"])
latin = sorted(c for c, l in langs.items() if not l["experimental"])
if not latin or not experimental:
    sys.exit("split-languages.py: a language package would be empty")
with open("latin.files", "w") as f:
    f.writelines(f"{path}/{c}.pfd\n" for c in latin)
with open("latin-experimental.files", "w") as f:
    f.writelines(f"{path}/{c}.pfd\n" for c in experimental)
with open("main-excludes.files", "w") as f:
    f.writelines(f"%exclude {path}/{c}.pfd\n" for c in latin + experimental)
print("parrotfish-keyboard-languages-latin:", " ".join(latin))
print("parrotfish-keyboard-languages-latin-experimental:", " ".join(experimental))
