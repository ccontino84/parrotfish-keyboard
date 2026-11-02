<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# Third-party material

| What | Where | Origin | Version | Licence | Our changes |
|---|---|---|---|---|---|
| AOSP LatinIME wordlists (en_US, it, de; es, fr, pt_BR, pt_PT, nl, sv, da, nb, ro; fi) and NOTICE | `data/aosp/` | `android.googlesource.com/platform/packages/inputmethods/LatinIME`, `dictionaries/` | commit `127336e9f29d69607eab55982324b210279ae8c5` (fetched 2026-10-08, the second group 2026-10-26, fi 2026-10-28; versions 44–54 from 2014) | Apache-2.0 (`LICENSES/Apache-2.0.txt`, `data/aosp/NOTICE`) | none; the build converts them to `.pfd` |
| Common Voice sentences | read by `tools/data/count-words.py`, not shipped; up to 2000 sentences per language in `tests/data/` | `github.com/common-voice/common-voice`, `server/data/<lang>/` | en, it, de: commit `a28a4cbf9188dafac0cb266f90bc9938f8418564` (2026-10-08); es, fr, pt, nl, sv-SE, da, nb-NO, ro: commit `4c9c6a9c9b0561e029f1d65448edaf62947e12ec` (2026-10-26); fi: the same commit (2026-10-28) | CC0-1.0 | random sample for tests; word counts |
| Tatoeba sentences | read by `tools/data/count-words.py`, not shipped | `downloads.tatoeba.org/exports/per_language/<lang>/` | eng, ita, deu: export downloaded 2026-10-08; spa, fra, por, nld, swe, dan, nob, ron: 2026-10-26; fin: 2026-10-28 | CC BY 2.0 FR (`LICENSES/LicenseRef-CC-BY-2.0-FR.txt`); some sentences CC0 | word counts only |
| Starting next-word data | `data/pairs/*.tsv.gz` (built into the `.pfd` files) | derived from the Common Voice and Tatoeba sentences by `tools/data/count-pairs.py` | — | LicenseRef-CC-BY-2.0-FR AND CC0-1.0 | generated counts of word pairs |
| Jolla keyboard `PreeditHandler.qml` (jolla-keyboard 0.10.4) | adapted in `handler/ParrotfishHandler.qml` | Sailfish OS 5.2.0.17 RPM `jolla-keyboard` (`/usr/lib64/qt5/qml/com/jolla/keyboard/`) | 0.10.4 | BSD-3-Clause (`LICENSES/BSD-3-Clause.txt`; the package's `LICENSE.BSD`) | our engine, autocorrect with undo, uncertain taps, key geometry, spacebar label |
| Frequency boosts | `data/rules/*.boost` | derived from the Tatoeba word counts by `tools/data/find-boosts.py` | — | LicenseRef-CC-BY-2.0-FR | generated list of words and calibrated (raised or lowered) frequencies |
| Tatoeba test sentences | `tests/data/tatoeba-*.txt` (tests only, not shipped in release builds) | Tatoeba exports of 2026-10-08 (en, it, de), 2026-10-28 (fi) and 2026-10-26 (the others) | — | LicenseRef-CC-BY-2.0-FR | up to 2000 random sentences per language; left out of all counts |
| Jolla keyboard layouts `en.qml`, `de.qml`, `fr.qml`, `fi.qml` (jolla-keyboard 0.10.4; long-press accents also from `da.qml`, `pt.qml`, `ro.qml`) | adapted in `layouts/parrotfish/arrangements.js` | Sailfish OS 5.2.0.17 RPM `jolla-keyboard-layout-western` | 0.10.4 | BSD-3-Clause (Jolla's notice kept in the files) | QWERTY: a full-width apostrophe key at the end of the middle row, `"` on B in the symbol view; QWERTZ: unchanged; AZERTY: the apostrophe in place of ç after N, ç on C's long press; QWERTY ÅÖÄ (fi.qml): unchanged; all: long-press æ ă on A, õ œ on O, ș on S, ț on T |
| Old spellings and their current forms | `data/rules/*.respellings` | derived from the AOSP lists, the `respell` rules and the Common Voice and Tatoeba word counts by `tools/data/find-respellings.py` | — | LicenseRef-CC-BY-2.0-FR AND CC0-1.0 | generated list of word pairs (old spellings from the corpora, as daß, paa, are words of those sentences) |
| Word supplement lists | `data/rules/*.supplement` | derived from the Common Voice and Tatoeba word counts by `tools/data/find-supplement.py` | — | LicenseRef-CC-BY-2.0-FR AND CC0-1.0 | generated list of words and frequencies |

Attribution for Tatoeba (to repeat on the About page): "Contains word
frequencies derived from sentences of the Tatoeba collection
(tatoeba.org), licensed under CC BY 2.0 FR."

Not used: FUTO code or data, keyboard-presage databases, XT9, OpenSubtitles,
Wikipedia dumps.
