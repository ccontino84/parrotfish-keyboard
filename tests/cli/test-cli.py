#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# parrotfish candidates / learned on a small learning.db (the
# schema of plugin/src/learningstore.cpp).
import os, sqlite3, subprocess, sys, tempfile

cli = sys.argv[1]
with tempfile.TemporaryDirectory() as tmp:
    db = os.path.join(tmp, "learning.db")
    c = sqlite3.connect(db)
    c.executescript("""
        CREATE TABLE words (lang TEXT NOT NULL, key TEXT NOT NULL, spelling TEXT NOT NULL,
            count REAL NOT NULL, is_new INTEGER NOT NULL, pending INTEGER NOT NULL,
            texts INTEGER NOT NULL, first_seen REAL NOT NULL, last_used REAL NOT NULL DEFAULT 0,
            PRIMARY KEY (lang, key));
        CREATE TABLE langs (lang TEXT PRIMARY KEY, typed REAL NOT NULL, next_fade REAL NOT NULL);
        INSERT INTO langs VALUES ('it', 12000, 20000), ('en', 300, 10000);
        INSERT INTO words VALUES ('it', 'mimmi', 'Mimmi', 4, 1, 1, 2, 11000, 11900);
        INSERT INTO words VALUES ('it', 'azzio', 'azzio', 1, 1, 1, 1, 6000, 6000);
        INSERT INTO words VALUES ('it', 'zorbel', 'zorbel', 3.2, 1, 0, 3, 0, 11800);
        INSERT INTO words VALUES ('it', 'casa', 'casa', 0.6, 0, 0, 0, 0, 1000);
        INSERT INTO words VALUES ('en', 'quorbix', 'quorbix', 1, 1, 1, 1, 100, 100);
    """)
    c.commit()
    c.close()

    def run(*args):
        r = subprocess.run([cli, *args, "--db", db], capture_output=True, text=True)
        assert r.returncode == 0, (args, r.returncode, r.stderr)
        return r.stdout

    out = run("candidates")
    print(out)
    lines = out.splitlines()
    mimmi = next(l for l in lines if "Mimmi" in l)
    assert "2/3" in mimmi and "4000 words" in mimmi, mimmi       # 11000 + 5000 - 12000
    azzio = next(l for l in lines if "azzio" in l)
    assert "starts over" in azzio, azzio                          # 6000 + 5000 < 12000
    assert "quorbix" in out and "zorbel" not in out

    out = run("candidates", "-l", "en")
    assert "quorbix" in out and "Mimmi" not in out

    out = run("learned")
    print(out)
    zorbel = next(l for l in out.splitlines() if "zorbel" in l)
    # 3.2 -> 2.56 -> 2.05 -> 1.64 -> 1.31 -> 1.05 -> 0.84 -> 0.67 -> 0.54 -> 0.43: 9 fadings,
    # the first in 8000 words, then 8 more every 10000.
    assert "3.2" in zorbel and "200 words ago" in zorbel and "after 9 fadings, in 88000 words" in zorbel, zorbel
    assert "casa" not in out

    out = run("learned", "--all")
    casa = next(l for l in out.splitlines() if "casa" in l)
    assert "casa*" in casa and "after 1 fading, in 8000 words" in casa, casa

    r = subprocess.run([cli, "learned", "--db", os.path.join(tmp, "missing.db")], capture_output=True, text=True)
    assert r.returncode == 1 and "cannot open" in r.stderr
    assert not os.path.exists(os.path.join(tmp, "missing.db"))
print("ok")
