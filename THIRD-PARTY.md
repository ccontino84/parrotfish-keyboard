<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Third-party material

| What | Licence | Our changes |
|---|---|---|
| AOSP LatinIME wordlists (`data/aosp/`, with AOSP's `NOTICE`), from android.googlesource.com/platform/packages/inputmethods/LatinIME | Apache-2.0 | converted into the dictionaries (`.pfd`), with words and frequencies added or adjusted from the sentence collections below |
| Sloleks Morphological Lexicon of Slovene 3.1 (Čibej, Gantar, Dobrovoljc, Krek, Holozan, Erjavec, Romih, Arhar Holdt, Krsnik, Robnik-Šikonja; hdl.handle.net/11356/2080, CLARIN.SI) | CC BY-SA 4.0 | its word forms with their Gigafida 2.0 counts, the 250,000 most frequent (names added automatically left out), as the Slovenian wordlist `data/sloleks/sl_wordlist.combined.gz` (`tools/data/sloleks-wordlist.py`), with frequencies on the AOSP scale; the Slovenian dictionary built from it (`sl.pfd`) is CC BY-SA 4.0 too |
| Kres corpus n-grams 2.0 (Kaja Dobrovoljc; hdl.handle.net/11356/1193, CLARIN.SI) | CC BY-SA 4.0 | its word-pair counts (2-grams), added to those of the sentence collections for the Slovenian next-word data `data/pairs/sl.tsv.gz` (`tools/data/count-pairs.py --bigrams`), which is CC BY-SA 4.0 too |
| Common Voice sentences (commonvoice.mozilla.org) | CC0-1.0 | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Tatoeba sentences (tatoeba.org) | CC BY 2.0 FR | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Data derived from both (`data/pairs/`, `data/rules/*.boost`, `*.supplement`, `*.respellings`), built into the dictionaries | CC BY 2.0 FR and CC0-1.0 | generated word pairs, frequencies and word lists |
| Jolla keyboard 0.10.4 (`PreeditHandler.qml`, layouts `en`, `de`, `fr`, `fi` and accents of `da`, `pt`, `ro`), Jolla Ltd. | BSD-3-Clause | adapted in `handler/ParrotfishHandler.qml` and `layouts/parrotfish/arrangements.js` (our engine, autocorrect and undo; an apostrophe key and accents per language); Jolla's notice kept in the files |

Contains word frequencies derived from sentences of the Tatoeba collection
(tatoeba.org), licensed under CC BY 2.0 FR, and of Mozilla Common Voice
(CC0). Contains wordlists of the Android Open Source Project (Apache-2.0).
The Slovenian wordlist, next-word data and dictionary are adapted from the
Sloleks Morphological Lexicon of Slovene 3.1 (hdl.handle.net/11356/2080)
and the Kres corpus n-grams 2.0 (hdl.handle.net/11356/1193), both licensed
under CC BY-SA 4.0, and are shared under the same licence.

The licence texts are in `LICENSES/`. Where each source was taken from
(commits, dates): DEVELOPMENT.md, "Dictionary data".
