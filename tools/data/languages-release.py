#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The dictionaries the settings download: data/dictionaries.json pins each
language's file in a GitHub release tagged languages-N, with its size and
SHA-256 (compressed and not). The dictionaries build the same on every
machine, so the built ones must match the pinned ones (ctest
dictionaries-pinned); a changed dictionary needs a new release.

  languages-release.py check DICTS_DIR PINNED_JSON
      every built dictionary is the pinned one
  languages-release.py prepare DICTS_DIR PINNED_JSON TAG OUT_DIR
      the changed and new dictionaries as CODE.pfd.gz in OUT_DIR, to
      publish as release TAG, and PINNED_JSON updated to point at them;
      unchanged ones keep their release

PINNED_JSON: {"base": URL of the releases' downloads, "languages": {CODE:
{"tag", "size", "sha256", "pfdSize", "pfdSha256"}}}
"""
import glob
import gzip
import hashlib
import json
import os
import sys


def built(dicts):
    out = {}
    for path in sorted(glob.glob(os.path.join(dicts, "*.pfd"))):
        with open(path, "rb") as f:
            out[os.path.basename(path)[:-4]] = f.read()
    return out


def load(pinned):
    if not os.path.exists(pinned):
        return {"base": "https://github.com/ccontino84/parrotfish-keyboard/releases/download", "languages": {}}
    with open(pinned, encoding="utf-8") as f:
        return json.load(f)


def check(dicts, pinned):
    langs = load(pinned)["languages"]
    problems = []
    for code, content in built(dicts).items():
        p = langs.get(code)
        if p is None:
            problems.append(f"{code}: not pinned")
        elif p["pfdSha256"] != hashlib.sha256(content).hexdigest():
            problems.append(f"{code}: differs from the one in {p['tag']}")
    for p in problems:
        print(p)
    if problems:
        sys.exit("Dictionaries changed: publish them (Languages workflow) and commit data/dictionaries.json")


def prepare(dicts, pinned, tag, out_dir):
    data = load(pinned)
    langs = data["languages"]
    os.makedirs(out_dir, exist_ok=True)
    for code, content in built(dicts).items():
        digest = hashlib.sha256(content).hexdigest()
        if code in langs and langs[code]["pfdSha256"] == digest:
            continue
        packed = gzip.compress(content, compresslevel=9, mtime=0)
        with open(os.path.join(out_dir, code + ".pfd.gz"), "wb") as f:
            f.write(packed)
        langs[code] = {"tag": tag, "size": len(packed), "sha256": hashlib.sha256(packed).hexdigest(),
                       "pfdSize": len(content), "pfdSha256": digest}
        print(f"{code}: {len(packed)} bytes in {tag}")
    data["languages"] = dict(sorted(langs.items()))
    with open(pinned, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)
        f.write("\n")


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "check":
        check(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 6 and sys.argv[1] == "prepare":
        prepare(*sys.argv[2:])
    else:
        sys.exit(__doc__)
