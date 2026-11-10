<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Adding a language

A language is more than a wordlist: it needs spelling rules, word lists
generated from public sentences and reviewed, test sentences, and benchmark
runs. Contributions are welcome as a pull request; say in it how well you
know the language. Building and testing are described in
[DEVELOPMENT.md](../DEVELOPMENT.md); the data files in its section
"Dictionary data".

**1. Check that the data exists.**

- A wordlist: the AOSP LatinIME one (`dictionaries/<lang>_wordlist.combined.gz`,
  Apache-2.0), or another list of words and frequencies converted to its
  format (Slovenian: the Sloleks lexicon, `tools/data/sloleks-wordlist.py`).
  Without one there is no dictionary.
- Public sentences for the word counts and word pairs: Common Voice
  (`server/data/<lang>/`, CC0) and Tatoeba
  (`downloads.tatoeba.org/exports/per_language/<iso639-3>/`, CC BY 2.0 FR).
  With little text (under about 1 MB in both), the language is marked
  experimental: next words are thin and the benchmarks less reliable.
- The letters on a keyboard: one of the arrangements in
  `layouts/parrotfish/arrangements.js`, with the language's letters on long
  press (`menu-letters`, below) and on its language keys (`extra-keys`:
  the most frequent ones, in the places of the language's own keyboard), or a new arrangement taken from Jolla's
  layout for it (BSD-3-Clause, as the others; note the source in the
  file's header comment).
- Other data sources (Hunspell, Wiktionary, UniMorph) depend on their
  licence:
  - CC BY-SA data, or GPL-3.0 data: allowed in files of their own. Every
    file built from them keeps that licence, with the source credited in
    THIRD-PARTY.md, so that our code stays LGPL.
  - GPL-2.0-only data: no. It can't be mixed with the Apache-2.0 AOSP lists.
  - Where a source offers several licences (many Hunspell dictionaries,
    Wiktionary's CC BY-SA or GFDL), take the most permissive one.
  - No licence, or non-commercial terms: no.
  - Custom terms: ask first, in an issue.

  These rules are not legal advice. Check the licence of what you add, and
  whether it is compatible, yourself, and say in the pull request what you
  checked.

**2. Add the files** (`xx` stands for the language code):

| File | What to put in it |
|---|---|
| `data/aosp/xx_wordlist.combined.gz` | the AOSP list, from the commit in DEVELOPMENT.md's "Dictionary data" table (or a newer one, noted there) |
| `data/rules/xx.language` | name, `experimental`, `menu-letters`, `extra-keys`, punctuation, `system-languages` (format: `tools/data/language-list.py`) |
| `data/rules/xx.rules` | spelling rules and generator settings (DEVELOPMENT.md, "Dictionary data"); for languages with many forms per word, `suffix` and `compound` rules |
| `data/rules/xx.keep`, `.lowercase`, `.shorthands`, `.paradigms` | optional, by hand (same table) |
| `data/CMakeLists.txt` | `xx:<AOSP name>` in `PARROTFISH_LANGUAGES` (`xx:<folder>/xx` for a list outside `data/aosp/`) |
| `tools/data/regenerate-lists.sh` | `xx` in `ALL`; its Common Voice folder in `cvdir` and Tatoeba code in `tatoeba` if they differ from `xx` |
| `tools/pf` | `xx` in `LANGUAGES`; `layout_of` and `language_group` if it isn't typed on QWERTY or goes with other languages |
| `tests/data/sentences-xx.txt`, `tatoeba-xx.txt` | test sentences drawn with `tools/data/sample-sentences.py` (commands in `tests/data/README.md`); note them in that README |

Letters that are letters of their own, not accented forms (sv `å ä ö`):
`no-misspellings` in `xx.rules`, so `har` and `här` both stay words.
`tools/data/make-oov-list.py` knows only `åäöæø` as such letters
(`OWN_LETTERS`), so extend it for others.

**3. Generate and review the lists.**

```sh
tools/data/regenerate-lists.sh ../corpora --fetch xx
git diff data/rules                # read every new list
```

- `xx.misspellings`: every plain spelling must really not be a word.
  Real words go into `xx.keep`, then regenerate.
- `xx.supplement`: names and foreign words go into `supplement.stop`.
- `xx.boost`: check the largest changes.

**4. Measure.**

```sh
tools/pf suite --label before      # once, before adding the language
tools/pf test                      # build, unit and QML tests
tools/pf suite --label xx          # includes the new language's cases
tools/pf compare before xx         # no other language may get worse
```

Also run `lang-bench` with the languages people mix it with, and with
close languages (es/pt, da/nb, cs/pl, hr/sl): the language must be told apart from them
(commands in DEVELOPMENT.md, "Build and test"). If many words of the test sentences are
unknown (Finnish: 10% before its rules), add `suffix`/`compound` rules or
mark the language experimental.

**5. Packaging and documentation.**

- The package is chosen from `experimental` in `xx.language`
  (`tools/split-languages.py`). Update the language names in the
  `%package` Summary and `%description` of `rpm/parrotfish-keyboard.spec`,
  in the README (feature list and install steps) and in `CHANGELOG.md`.
- Record the sources in the table under "Dictionary data" in DEVELOPMENT.md (the AOSP list,
  the Common Voice commit, the Tatoeba export date).
- Data under another licence (CC BY-SA): its files in `REUSE.toml`, the
  licence text in `LICENSES/`, a row in THIRD-PARTY.md, and the licence in
  the language package's `License:` in the spec.
- Settings translation (`translations/parrotfish-keyboard-xx.ts`):
  welcome, but separate from the language itself.
