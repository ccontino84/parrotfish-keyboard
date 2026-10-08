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
| `plugin/` | QML module `Parrotfish.Keyboard` (Qt 5.6): the engine as the list model Jolla's `PreeditHandler` expects |
| `handler/` | `ParrotfishHandler.qml` and its `.conf` (registered only for our layout's code, PARROTFISH; Jolla's layouts keep XT9 or Hunspell) |
| `layouts/` | the one "Auto (Parrotfish)" layout (`parrotfish.qml`, using our handler through `handler=` in `layouts_parrotfish.conf`; the name sorts it to the top of Jolla's keyboard list); its letter rows are built from `parrotfish/arrangements.js` (QWERTY: Jolla's en keys plus a full-width apostrophe ending the middle row; QWERTZ: Jolla's de keys), picked by `/apps/parrotfish-keyboard/layout`; optional key backgrounds in `parrotfish/` |
| `settings/` | our section in Settings → Text input: `settings=` in both `.conf` files points to it, as XT9's does |
| `rpm/` | RPM spec |
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

Running text, as on the phone (all languages, previous words as context):

```sh
build/tests/sentence-bench -l data/layouts/en.layout -t tests/data/tatoeba-it.txt \
    -d build/data/it.pfd -d build/data/en.pfd -d build/data/de.pfd [-P strength=2]
```

`-P name=value` overrides any engine parameter (`EngineParams`,
`CorrectorParams`, `LanguageParams`); the defaults in the headers come from
these benchmarks.

Sanitizers: configure a second tree with
`-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`.

## Phone build (RPM)

```sh
sfdk config target=SailfishOS-5.1.0.11-aarch64
sfdk build                 # RPM in RPMS/
```

Local builds carry a pre-release suffix in the spec's `Version`
(`0.1.0~alpha1`); only CI release builds have the plain version.
`PARROTFISH_DEVTOOLS=ON` (test builds) also installs `pf-query`,
`typo-bench` and `lang-bench` in `/usr/libexec/parrotfish-keyboard/`.

How it runs: `ParrotfishHandler.qml` is Jolla's `PreeditHandler` with our
`PredictionEngine` as its `engine`. When a word starts, the handler passes
the letter keys of the current layout (centres and sizes) and the text
before the cursor; then each change of the word being typed updates the
suggestions. Autocorrect strength is the dconf key
`/apps/parrotfish-keyboard/autocorrect` (`off`, `gentle`, `normal`); hidden
swiping to another layout can be switched off:
`/apps/parrotfish-keyboard/swipe` (`normal`, the default; `off` = a full keyboard width; long-press the
spacebar to pick a layout). An empty field leans towards the language the
last text was in: `/apps/parrotfish-keyboard/last-language` (written by the
handler). `/apps/parrotfish-keyboard/dividers` (`true`, the default;
`false`) switches the dividers between suggestions. `/apps/parrotfish-keyboard/layout` (`qwerty`, the default;
`qwertz`) picks the keys. The look of the keys of our layouts
(`layouts/parrotfish/`): `/apps/parrotfish-keyboard/key-fill` (`none` = the
default, `flat`, `shaded`) and `/apps/parrotfish-keyboard/key-border`
(`none` = the default, `line`, `raised` = a rim lit from above, drawn by a
shader), in any combination; both `none` is Jolla's look. Letter keys stay
fainter than the spacebar. Hidden strengths:
`/apps/parrotfish-keyboard/key-tuning`, e.g.
`"'fill=0.03 shade=0.05 border=0.1'"`. All of these are in the settings section (`settings/`). Hidden tuning is `/apps/parrotfish-keyboard/tuning`, a string of `name=value`
pairs for any engine parameter (`dconf write
/apps/parrotfish-keyboard/tuning "'gentleConfidence=0.4 pairWeight=0.6'"`).
With `~/.local/share/parrotfish-keyboard/debug` present, the
plugin logs timings (never the typed text) to the journal.

Plugin test on the PC: install to a staging dir (`DESTDIR=… cmake --install
build`) and load it in `qmlscene-qt5` with `QML2_IMPORT_PATH` and
`PARROTFISH_DATA_DIR` pointing there.

## Dictionary data

`tools/build-dict.py` turns an AOSP wordlist into a memory-mapped `.pfd`
file (format: `engine/include/parrotfish/dictformat.h`). It reads, from
`data/rules/`:

| File | Content | Made by |
|---|---|---|
| `<lang>.rules` | spelling rules (`ae ä`, `e' è final`) and settings for the generators | hand |
| `<lang>.misspellings` | accent-less spellings that are not words (`perche` → `perché`) | `tools/find-accent-misspellings.py`, reviewed |
| `<lang>.supplement` | common words missing from the AOSP list (`uomo`, `überhaupt`) | `tools/find-supplement.py`, reviewed |
| `<lang>.boost` | raised frequencies of everyday words (`penso`, `voglio`) | `tools/find-boosts.py`, reviewed |
| `<lang>.paradigms`, `<lang>.paradigm-skip` | regular verb endings, irregular verbs to leave alone | hand |
| `<lang>.forms` | missing forms of regular verbs (`riuscivi`) | `tools/complete-paradigms.py`, review a sample |
| `supplement.stop` | Tatoeba example names and over-used words, kept out of all generated data | hand |
| `../pairs/<lang>.tsv.gz` | starting next-word data (word pairs) | `tools/count-pairs.py` |

The generated lists need the sentence corpora (Common Voice, CC0; Tatoeba,
CC BY 2.0 FR; about 400 MB), which are not in the repo. To regenerate the
lists and the test lists of unknown real words (`build/data/oov-*.txt`):

```sh
tools/regenerate-lists.sh ~/corpora --fetch    # without --fetch: reuse ~/corpora
git diff data/rules                            # review before committing
```
