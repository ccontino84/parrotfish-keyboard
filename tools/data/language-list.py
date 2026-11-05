#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The language list the keyboard installs (languages.json), from each
language's data/rules/LANG.language: one place per language for what the
plugin's Languages singleton tells the layout and the settings.

LANG.language, one setting a line (# comments):
  name NAME                  its name in the language ("Deutsch")
  experimental               too little text to build and test its data well
  menu-letters LETTERS       the accented letters its long-press menus show
                             first (about 0.05 % of everyday words or more)
  comma-extra SIGNS          signs added to the comma's long press (—)
  period-before SIGNS        signs before ! . ? in the period's (¡)
  period-after SIGNS         ... and after them (¿)
  system-languages CODES     system language codes that mean this one
                             (nb: no nn; pt_PT: pt)
  fallback                   the language when the system's has none (en)

usage: language-list.py RULES_DIR OUT.json LANG...
"""
import json
import sys


def read(path):
    out = {"name": "", "experimental": False, "menuLetters": "", "commaExtra": "", "periodBefore": "",
           "periodAfter": "", "systemLanguages": [], "fallback": False}
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            key, _, value = line.partition(" ")
            value = value.strip()
            if key == "name":
                out["name"] = value
            elif key in ("experimental", "fallback"):
                out[key] = True
            elif key in ("menu-letters", "comma-extra", "period-before", "period-after"):
                parts = key.split("-")
                out[parts[0] + parts[1].capitalize()] = value.replace(" ", "")
            elif key == "system-languages":
                out["systemLanguages"] = value.split()
            else:
                sys.exit(f"{path}:{n}: unknown setting {key!r}")
    if not out["name"]:
        sys.exit(f"{path}: no name")
    return out


def main():
    rules, out, langs = sys.argv[1], sys.argv[2], sys.argv[3:]
    data = {lang: read(f"{rules}/{lang}.language") for lang in langs}
    if sum(1 for d in data.values() if d["fallback"]) != 1:
        sys.exit("language-list.py: exactly one language must be the fallback")
    with open(out, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=1, sort_keys=True)
        f.write("\n")


if __name__ == "__main__":
    main()
