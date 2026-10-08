<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# Test data

`sentences-{en,it,de}.txt`: 2000 sentences each, drawn at random (seed
20261008) from the Mozilla Common Voice sentence files
(`github.com/common-voice/common-voice`, `server/data/<lang>/*.txt`, commit
`a28a4cbf9188dafac0cb266f90bc9938f8418564`; CC0-1.0). Only sentences of
6–25 words without digits or markup were kept. `DictionaryofForeignPhrases.txt`,
`countries-and-cities.txt` and `german-cities.txt` were skipped, because
they're lists of foreign words or names rather than running text.

Used by `tests/bench/lang-bench.cpp` to build mixed-language texts.

`tatoeba-{en,it,de}.txt`: 2000 sentences each of 4–20 words, drawn at
random (seed 20261008) from the Tatoeba per-language exports downloaded on
2026-10-08 (CC BY 2.0 FR). Everyday language, unlike the Common Voice
sentences. Used by `tests/bench/sentence-bench.cpp`.

Both sets are left out of every word count (`--exclude` in
`tools/regenerate-lists.sh`), so the benchmarks don't test on data the
dictionaries were built from.
