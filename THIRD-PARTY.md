<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# Third-party material

| What | Where | Origin | Version | Licence | Our changes |
|---|---|---|---|---|---|
| AOSP LatinIME wordlists (en_US, it, de) and NOTICE | `data/aosp/` | `android.googlesource.com/platform/packages/inputmethods/LatinIME`, `dictionaries/` | commit `127336e9f29d69607eab55982324b210279ae8c5` (fetched 2026-10-08; the files are version 54 from 2014) | Apache-2.0 (`LICENSES/Apache-2.0.txt`, `data/aosp/NOTICE`) | none; the build converts them to `.pfd` |
| Common Voice sentences | read by `tools/count-words.py`, not shipped; 2000 sentences per language in `tests/data/` | `github.com/common-voice/common-voice`, `server/data/{en,it,de}/` | commit `a28a4cbf9188dafac0cb266f90bc9938f8418564` (2026-10-08) | CC0-1.0 | random sample for tests; word counts |
| Tatoeba sentences | read by `tools/count-words.py`, not shipped | `downloads.tatoeba.org/exports/per_language/{eng,ita,deu}/` | export downloaded 2026-10-08 | CC BY 2.0 FR (`LICENSES/LicenseRef-CC-BY-2.0-FR.txt`); some sentences CC0 | word counts only |
| Word supplement lists | `data/rules/*.supplement` | derived from the Common Voice and Tatoeba word counts by `tools/find-supplement.py` | — | LicenseRef-CC-BY-2.0-FR AND CC0-1.0 | generated list of words and frequencies |

Attribution for Tatoeba (to repeat on the About page): "Contains word
frequencies derived from sentences of the Tatoeba collection
(tatoeba.org), licensed under CC BY 2.0 FR."

Not used: FUTO code or data, keyboard-presage databases, XT9, OpenSubtitles,
Wikipedia dumps.
