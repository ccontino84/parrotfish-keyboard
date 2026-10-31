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

The other languages (es, fr, pt, nl, sv, da, nb, ro; pt serves pt_BR and
pt_PT) were drawn the same way by `tools/sample-sentences.py` from Common
Voice commit `4c9c6a9c9b0561e029f1d65448edaf62947e12ec` and the Tatoeba
exports of 2026-10-26: at most 2000, and at most 5% of the sentences that
qualify, so small corpora keep enough for the word counts (sentences-nb
has 109, sentences-ro 673, tatoeba-nb 796). fi was drawn the same way on
2026-10-28, from the same Common Voice commit and the Tatoeba export of
that day (sentences-fi has 151):

```sh
tools/sample-sentences.py tests/data/sentences-es.txt ~/corpora/cv/server/data/es/*.txt
tools/sample-sentences.py tests/data/tatoeba-es.txt ~/corpora/spa_sentences.tsv.bz2 --min-words 4 --max-words 20
```

(Leave out the Common Voice files that `cvfiles` in
`tools/regenerate-lists.sh` skips.)

Both sets are left out of every word count (`--exclude` in
`tools/regenerate-lists.sh`), so the benchmarks don't test on data the
dictionaries were built from.

`alnum.txt`: tokens with one digit after a letter (mp3, ps4, h2o, win7),
written for the project. With a number row they are typed as one word;
`sentence-bench -N 75 -o tests/data/alnum.txt` checks that autocorrect
leaves them alone.
