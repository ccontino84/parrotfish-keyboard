<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# Parrotfish development

## Layout

| Path | What |
|---|---|
| `engine/` | the typing engine: plain C++17, no Qt, so it builds and tests on the PC |
| `engine/tools/pf-query.cpp` | try the corrector by hand |
| `tools/` | Python tools that build and maintain the dictionary data |
| `data/aosp/` | AOSP wordlists (Apache-2.0), unchanged |
| `data/rules/` | per-language rules and reviewed generated lists (below) |
| `data/layouts/` | approximate letter rows of Jolla's en/it/de layouts, for PC tests |
| `tests/unit/` | unit tests (ctest) |
| `tests/bench/` | accuracy benchmarks: typos (`typo-bench`) and mixed languages (`lang-bench`) |

## Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build            # engine, dictionaries (build/data/*.pfd), tests
(cd build && ctest --output-on-failure)
```

Try words: `build/engine/pf-query -l data/layouts/it.layout -d build/data/it.pfd perche citta\'`

Benchmarks:

```sh
build/tests/typo-bench -l data/layouts/de.layout -d build/data/de.pfd [-d build/data/en.pfd] \
    [-o build/data/oov-de.txt] [-n 1000] [-P gentleConfidence=0.7]
build/tests/lang-bench -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd [-P switchProb=0.01]
```

`-P name=value` overrides any engine parameter (`EngineParams`,
`CorrectorParams`, `LanguageParams`); the defaults in the headers come from
these benchmarks.

Sanitizers: configure a second tree with
`-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`.

## Dictionary data

`tools/build-dict.py` turns an AOSP wordlist into a memory-mapped `.pfd`
file (format: `engine/include/parrotfish/dictformat.h`). It reads, from
`data/rules/`:

| File | Content | Made by |
|---|---|---|
| `<lang>.rules` | spelling rules (`ae ä`, `e' è final`) and settings for the generators | hand |
| `<lang>.misspellings` | accent-less spellings that are not words (`perche` → `perché`) | `tools/find-accent-misspellings.py`, reviewed |
| `<lang>.supplement` | common words missing from the AOSP list (`uomo`, `überhaupt`) | `tools/find-supplement.py`, reviewed |
| `supplement.stop` | Tatoeba example names kept out of the supplement | hand |

The generated lists need the sentence corpora (Common Voice, CC0; Tatoeba,
CC BY 2.0 FR; about 400 MB), which are not in the repo. To regenerate the
lists and the test lists of unknown real words (`build/data/oov-*.txt`):

```sh
tools/regenerate-lists.sh ~/corpora --fetch    # without --fetch: reuse ~/corpora
git diff data/rules                            # review before committing
```
