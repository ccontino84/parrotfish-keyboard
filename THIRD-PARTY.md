<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Third-party material

| What | Licence | Our changes |
|---|---|---|
| AOSP LatinIME wordlists (`data/aosp/`, with AOSP's `NOTICE`), from android.googlesource.com/platform/packages/inputmethods/LatinIME | Apache-2.0 | converted into the dictionaries (`.pfd`), with words and frequencies added or adjusted from the sentence collections below |
| Common Voice sentences (commonvoice.mozilla.org) | CC0-1.0 | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Tatoeba sentences (tatoeba.org) | CC BY 2.0 FR | word and word-pair counts; random samples as test sentences (`tests/data/`) |
| Data derived from both (`data/pairs/`, `data/rules/*.boost`, `*.supplement`, `*.respellings`), built into the dictionaries | CC BY 2.0 FR and CC0-1.0 | generated word pairs, frequencies and word lists |
| Jolla keyboard 0.10.4 (`PreeditHandler.qml`, layouts `en`, `de`, `fr`, `fi` and accents of `da`, `pt`, `ro`), Jolla Ltd. | BSD-3-Clause | adapted in `handler/ParrotfishHandler.qml` and `layouts/parrotfish/arrangements.js` (our engine, autocorrect and undo; an apostrophe key and accents per language); Jolla's notice kept in the files |

Contains word frequencies derived from sentences of the Tatoeba collection
(tatoeba.org), licensed under CC BY 2.0 FR, and of Mozilla Common Voice
(CC0). Contains wordlists of the Android Open Source Project (Apache-2.0).

The licence texts are in `LICENSES/`. Where each source was taken from
(commits, dates): DEVELOPMENT.md, "Dictionary data".
