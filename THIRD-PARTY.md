<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Third-party material

| What | Licence | Our changes |
|---|---|---|
| AOSP LatinIME wordlists (`data/aosp/`, with AOSP's `NOTICE`), from android.googlesource.com/platform/packages/inputmethods/LatinIME | Apache-2.0 | converted into the dictionaries (`.pfd`), with words and frequencies added or adjusted from the sentence collections below |
| Sloleks Morphological Lexicon of Slovene 3.1 (Čibej, Gantar, Dobrovoljc, Krek, Holozan, Erjavec, Romih, Arhar Holdt, Krsnik, Robnik-Šikonja; hdl.handle.net/11356/2080, CLARIN.SI) | CC BY-SA 4.0 | its word forms with their Gigafida 2.0 counts, the 250,000 most frequent (names added automatically left out), as the Slovenian wordlist `data/sloleks/sl_wordlist.combined.gz` (`tools/data/sloleks-wordlist.py`), with frequencies on the AOSP scale; the Slovenian dictionary built from it (`sl.pfd`) is CC BY-SA 4.0 too |
| Leipzig Corpora Collection, Universität Leipzig / InfAI (wortschatz-leipzig.de; news corpora `ukr_news_2020_1M`, `_2022_1M`, `_2023_1M`, `_2024_1M`, `ukr_newscrawl_2011_1M`) | CC BY (Leipzig's terms name no version; CC BY 4.0 assumed) | word counts of their sentences, up to 250,000 of the commonest word forms (Latin-script words, Russian quotes, rare acronyms left out) as the Ukrainian wordlist `data/leipzig/uk_wordlist.combined.gz` (`tools/data/leipzig-wordlist.py`), with frequencies on the AOSP scale; the Ukrainian dictionary built from it (`uk.pfd`) |
| Kres corpus n-grams 2.0 (Kaja Dobrovoljc; hdl.handle.net/11356/1193, CLARIN.SI) | CC BY-SA 4.0 | its word-pair counts (2-grams), added to those of the sentence collections for the Slovenian next-word data `data/pairs/sl.tsv.gz` (`tools/data/count-pairs.py --bigrams`), which is CC BY-SA 4.0 too |
| Common Voice sentences (commonvoice.mozilla.org) | CC0-1.0 | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Tatoeba sentences (tatoeba.org) | CC BY 2.0 FR | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Data derived from both (`data/pairs/`, `data/rules/*.boost`, `*.supplement`, `*.respellings`), built into the dictionaries | CC BY 2.0 FR and CC0-1.0 | generated word pairs, frequencies and word lists |
| Jolla keyboard 0.10.4 (`PreeditHandler.qml`, layouts `en`, `de`, `fr`, `fi`, `el`, `ru` (as on Sailfish OS 5.1) and accents of `da`, `pt`, `ro`), Jolla Ltd. | BSD-3-Clause | adapted in `handler/ParrotfishHandler.qml` and `layouts/parrotfish/arrangements.js` (our engine, autocorrect and undo; an apostrophe key and accents per language; the sticky key in the bottom row of `ru` and `el`); Jolla's notice kept in the files |
| Jolla keyboard's Ukrainian layout `uk.qml`, Oleksii Serdiuk and Jolla Ltd. | BSD-3-Clause | its rows in `layouts/parrotfish/arrangements.js`, shown on the ЙЦУКЕН layout while typing Ukrainian; the notice kept in the file |

The Ukrainian wordlist is derived from corpora of the
Leipzig Corpora Collection (Universität Leipzig / InfAI,
wortschatz-leipzig.de), licensed under CC BY.

Contains word frequencies derived from sentences of the Tatoeba collection
(tatoeba.org), licensed under CC BY 2.0 FR, and of Mozilla Common Voice
(CC0). Contains wordlists of the Android Open Source Project (Apache-2.0).
The Slovenian wordlist, next-word data and dictionary are adapted from the
Sloleks Morphological Lexicon of Slovene 3.1 (hdl.handle.net/11356/2080)
and the Kres corpus n-grams 2.0 (hdl.handle.net/11356/1193), both licensed
under CC BY-SA 4.0, and are shared under the same licence.

## Voice input (aarch64 builds)

| What | Licence | Our changes |
|---|---|---|
| sherpa-onnx 2.0.1 C API, prebuilt for aarch64 (github.com/k2-fsa/sherpa-onnx) | Apache-2.0 | none; linked by the helper `parrotfish-voice` |
| ONNX Runtime 1.28.2, as bundled by sherpa-onnx (github.com/microsoft/onnxruntime), Microsoft Corporation | MIT, with its third-party notices (`voice/licenses/`) | none |
| Silero VAD (`silero_vad.onnx`, github.com/snakers4/silero-vad), Silero Team | MIT | none |
| Speech model, downloaded by the user from the settings, not shipped: NVIDIA Parakeet TDT 0.6B v3, post-trained by Moondream (parakeet-ultra), converted to int8 ONNX by the sherpa-onnx project (huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-ultra-int8) | CC BY 4.0 | none; credited on the settings page |

The helper `parrotfish-voice` is our code (LGPL-2.1-or-later) linked with
sherpa-onnx (Apache-2.0); Apache-2.0 is compatible with version 3 of the
LGPL only, so the helper binary is distributed under LGPL-3.0-or-later, as
the "or later" permits (`voice/licenses/LGPL-3.0.txt` and `GPL-3.0.txt`,
beside ONNX Runtime's and Silero VAD's notices). The downloader
`parrotfish-download` uses only Qt and the system's zlib.

The licence texts are in `LICENSES/`. Where each source was taken from
(commits, dates): DEVELOPMENT.md, "Dictionary data".
