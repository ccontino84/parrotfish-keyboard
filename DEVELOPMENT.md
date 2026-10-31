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
| `layouts/` | the one "Auto (Parrotfish)" layout (`parrotfish.qml`, using our handler through `handler=` in `layouts_parrotfish.conf`; the name sorts it to the top of Jolla's keyboard list); its letter rows are built from `parrotfish/arrangements.js` (QWERTY: Jolla's en keys plus a full-width apostrophe ending the middle row; QWERTZ: Jolla's de keys; AZERTY: Jolla's fr keys with the apostrophe in place of ç, which moves to C's long press; QWERTY ÅÖÄ: Jolla's fi keys), picked by `/apps/parrotfish-keyboard/layout`; optional key backgrounds in `parrotfish/` |
| `settings/` | our section in Settings → Text input: `settings=` in both `.conf` files points to it, as XT9's does; the typing languages have their own page (`ParrotfishLanguages.qml`, list in `languages.js`) |
| `rpm/` | RPM spec |
| `tools/pf` | the developer front door (below): tests, lint, benchmark suite, comparisons, RPM |
| `tools/data/` | Python tools that build and maintain the dictionary data |
| `tools/eval/` | benchmark helpers: parameter sweeps, mixed-language texts |
| `tools/qt56-scan.sh` | flags QML and JavaScript that the phone's Qt 5.6 rejects |
| `data/aosp/` | AOSP wordlists (Apache-2.0), unchanged |
| `data/rules/` | per-language rules and reviewed generated lists (below) |
| `tests/data/` | held-out test sentences per language (`README.md` there) |
| `data/layouts/` | approximate letter rows of Jolla's en/it/de layouts, for PC tests (square keys); `phone-*.layout`: keys as tall as on the phone (`pitch`; AZERTY and Nordic by the same geometry, not logged) |
| `tests/unit/` | unit tests (ctest) |
| `tests/qml/` | the handler on the PC with the plugin, Sailfish OS's modules replaced by stand-ins (ctest `qml`, needs `qmltestrunner`) |
| `tests/bench/` | accuracy benchmarks: typos and autocorrect in running text (`sentence-bench`), next words (`nextword-bench`), single words (`typo-bench`), mixed languages (`lang-bench`) |

## Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build            # engine, dictionaries (build/data/*.pfd), tests
(cd build && ctest --output-on-failure)
```

The front door, `tools/pf` (Python, standard library only; `tools/pf -h`):

```sh
tools/pf lint                   # QML/JS that Qt 5.6 rejects
tools/pf test                   # build, unit and QML tests
tools/pf suite                  # the standard benchmark suite, stored with its commit
tools/pf suite --label try --param otherLanguageMargin=8   # an experiment
tools/pf compare alpha70 try    # two runs side by side, better/WORSE marked
tools/pf check                  # before a release: lint, tests, suite, and
                                # exit 1 if a number got worse than the last run
tools/pf rpm --version 0.1.0~alpha71   # phone RPM (sfdk), copied if configured
```

The suite: typos in running text (en, it, de; fr, es) with the number row,
uncertain taps and names mixed in; codes with a digit; words of another
language in mixed texts; next words and keystrokes saved. Settings of one
machine are in `~/.config/parrotfish-dev.ini` (`tools/pf config`): where
results go (outside the repo), where `pf rpm` copies the RPM, the sfdk
target, and more test texts of your own for the next-word cases (`[eval]`,
`NAME = LANG:PATH`, one message per line).

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

Each sentence is a new text; `-m it` starts it leaning towards a language as
the plugin does (the last text's), `-B` adds the first word of a text and
2-letter words separately, `-F` types only the first words. `-R` makes
the typos real words (wad for was; for Strong, `-P strength=3`): the
`real word` row counts them fixed, the `clean` row the correct words
changed.

Closer to real typing (sentence-bench uses Normal, as the phone; wrong
keys go mostly sideways, 10% up or down):

```sh
build/tests/sentence-bench -l data/layouts/phone-qwerty.layout -t tests/data/sentences-en.txt -m en \
    -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd \
    -X -N 75 -o build/data/oov-en.txt [-S slips.log [-M]]
tools/eval/bench-sweep.sh normalConfidence "0.3 0.5 0.7 0.85" ...same arguments...
```

`-X` adds a missing space (`thisis`), the space typed as a key above the
spacebar (`thisbis`) and, with `-N 75` (a number row 75% tall), a top-row
letter typed as the digit above it (`h3llo`; a leading digit is typed as
it is, as the handler does). `-o` mixes in words no loaded dictionary knows
(names), which must stay as typed; with `-N`, `-o tests/data/alnum.txt`
does the same for codes with a digit (`mp3`, `ps4`). `-S` takes the slip
directions from a slips.log (one person's hands; `-M` mirrors them, as for
the other hand). `-T` adds taps that touched one key and lifted on its neighbour (shown
as the handler would; `-K` also gives the engine the other key, as the
phone does since alpha70). `-L` adds the results by typed length (2, 3–4, 5+
letters), as used for the thresholds by length. The last line sums up, with the word error rate before and
after autocorrect; `bench-sweep.sh` prints it for each value of a parameter.

Next words and completions (the word-pair data):

```sh
build/tests/nextword-bench -l data/layouts/phone-qwerty.layout -t tests/data/tatoeba-en.txt -m en \
    -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd [-v]
BENCH=build/tests/nextword-bench tools/eval/bench-sweep.sh pairWeight "0.3 0.5 0.7" ...same arguments...
```

Before each word, with the line up to it as context, it counts the word
among the next-word suggestions (top 1, top 3; the first word of a line
apart) and the share of words no dictionary knows. Then it types each word
letter by letter, without typos, and picks it once it is among the first
three suggestions (one tap, with the space): the keystrokes saved (KSR) are
1 − taps / (letters + spaces). `-v` prints the words the top 3 missed.
Tatoeba's example names (Tom, Mary; `data/rules/supplement.stop`) have no
word pairs, so they and the words after them are mostly misses.

Mixed-language texts, for rules that concern other languages' words:
`tools/eval/mix-sentences.py insert tests/data/tatoeba-en.txt
tests/data/tatoeba-it.txt > mix.txt` puts one Italian word into each English
sentence (`switch`: an English sentence, then an Italian one); run
sentence-bench on it with `-m en -r 0 -v` and count the `changed` lines:
every one is a word of the other language changed by autocorrect.

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
`/apps/parrotfish-keyboard/autocorrect` (`normal`, the default; `gentle`,
`off`; `strong`, experimental: `normal`, and after a previous word a valid
word one slip away from a far likelier one is fixed too, `strongMargin`,
plan §6); hidden swiping to another layout can be switched off:
`/apps/parrotfish-keyboard/swipe` (`normal`, the default; `off` = a full keyboard width). An empty field leans
towards the language the last text was in:
`/apps/parrotfish-keyboard/last-language` (written by the handler).
`/apps/parrotfish-keyboard/languages` holds the typing languages, codes
separated by spaces (`"en it de"`); empty, the default, means the system
language only (English if there is no dictionary for it); others are added
in the settings. Every enabled language is searched on each keystroke, the
likeliest first, the others only for words that can still compete
(`searchMargin`) (settings page: `settings/ParrotfishLanguages.qml`).
`/apps/parrotfish-keyboard/dividers` (`true`, the default; `false`) switches
the dividers between suggestions. `/apps/parrotfish-keyboard/layout`
(`qwerty`, `qwertz`, `azerty`, `nordic` = QWERTY with å ö ä keys as on
Jolla's Finnish and Swedish layouts) picks the keys. Unset, it follows the
system locale (`preferred()` in `layouts/parrotfish/arrangements.js`, the
same as `layoutFor()` in `settings/languages.js`): de and fr_CH QWERTZ, fr
AZERTY (fr_CA QWERTY), fi and sv QWERTY ÅÖÄ, otherwise QWERTY; the settings
show that choice until another is picked. The look of the
keys of our layouts (`layouts/parrotfish/`):
`/apps/parrotfish-keyboard/key-fill` (`none` = the default, `flat`,
`shaded`) and `/apps/parrotfish-keyboard/key-border` (`none` = the default,
`line`, `raised` = a rim lit from above, drawn by a shader), in any
combination; both `none` is Jolla's look. `/apps/parrotfish-keyboard/key-height`
(`100` = the default, `90`, `80`) shortens every row of our layout in
portrait, in percent of Jolla's height (`layouts/parrotfish.qml` replaces
`keyHeight` as Jolla's `updateSizes()` sets it). `/apps/parrotfish-keyboard/number-row`
(`false` = the default, `true`) adds a row of digits above the letters
(`withNumberRow()` in `layouts/parrotfish/arrangements.js`): the digit row of
the first symbol page becomes that row; the first symbol page is then the
second page's first two rows over the first page's other two, and the second
page holds rare signs (fractions, quotes, maths, ©).
`/apps/parrotfish-keyboard/number-row-height`: its height in percent of the
other rows (`100` = the default; the settings switch "Shorter number row"
writes `75`; any value from 40 works). The apostrophe key's
long press offers `"` and `’`. Letter keys stay fainter than the
spacebar. Hidden strengths: `/apps/parrotfish-keyboard/key-tuning`, e.g.
`"'fill=0.03 shade=0.05 border=0.1 gap=0.05'"` (`gap`: the space around
each key background, across and down, as a share of the row height; also
for Jolla's backgrounds of `?123`, the spacebar and Enter). All of these are in the settings
section (`settings/`). Hidden tuning is `/apps/parrotfish-keyboard/tuning`,
a string of `name=value` pairs for any engine parameter (`dconf write
/apps/parrotfish-keyboard/tuning "'gentleConfidence=0.4 pairWeight=0.6'"`).
With `~/.local/share/parrotfish-keyboard/debug` present, the plugin logs
timings (never the typed text) to the journal, and appends to
`~/.local/share/parrotfish-keyboard/slips.log` the kind of slip behind every
autocorrection kept or undone, every correction picked from the bar and
every word changed by hand (Backspace back into a committed word):
`kept key k l` (typed k, meant l), `undone key j h`, `undone late key g h`
(an autocorrection changed by hand after more typing; it also has a
`kept` line), `manual key r e` (a word the engine left alone, fixed by
hand; a quite different word is not logged), `picked swap`,
`strong kept key d s margin=9.0` (a valid word fixed by Strong, with its
margin in nats; also `strong undone`), `missing`,
`missing-double`, `extra`, `extra-double`, `accent`, `other`. Letter pairs
only, never words; it is the real tap data for the key model (which slips
are common on the phone). A header line names the settings the lines below
come from, written again whenever they change, each day and after a
restart: `# 2026-10-29 0.1.0~alpha51 QWERTY portrait height=90 normal
pitch=1.67 tuning=keyHeightShare=0.2` (date, version, arrangement,
orientation, key height %, strength, rows apart in key widths as measured,
hidden tuning if any). `words 50` lines count the words typed letter by
letter under that header (in batches of 50, and the rest when the settings
change; up to 49 are lost at a restart), so slips can be compared per word.

The undo memory (typed words whose autocorrection was undone; two undos
stop it) is kept between sessions in
`~/.local/share/parrotfish-keyboard/learning.db` (SQLite, the system's
`sqlite-libs`), whether or not the debug file is there: words, so no
debug data. Every table has a cap (undos: the 1,000 used last) and freed
space is given back. Deleting the file, with the keyboard closed (or
before `systemctl --user restart maliit-server`), forgets everything.

Plugin test on the PC: install to a staging dir (`DESTDIR=… cmake --install
build`) and load it in `qmlscene-qt5` with `QML2_IMPORT_PATH` and
`PARROTFISH_DATA_DIR` pointing there.

## Dictionary data

`tools/data/build-dict.py` turns an AOSP wordlist into a memory-mapped `.pfd`
file (format: `engine/include/parrotfish/dictformat.h`). It reads, from
`data/rules/`:

| File | Content | Made by |
|---|---|---|
| `<lang>.rules` | spelling rules (`ae ä`, `e' è final`, `ş ș`); settings for the generators: `paradigm`/`paradigm-plain` (real verb forms that are not accent misspellings), `only-marks`, `no-misspellings`, `capitalised-nouns`, `min-freq` (drop the padding of a list; ro), `one-source` (a supplement word needs only N sightings in both corpora together; fi); word forms the list lacks, for the engine (`Dictionary::isWordForm`; fi): `suffix ENDING... [after=LETTERS] [drop=LETTERS]` (endings on a word: kännykkä → kännykkänsä; `drop`: talon → taloni) and `compound MIN PARTS` (up to PARTS words of at least MIN letters, and hyphenated parts). Such forms stand when typed (never autocorrected, and count for their language) but are never suggested | hand |
| `<lang>.keep` | plain spellings that are words of their own, never marked as misspellings (pt `forca`, ro `frumoasa`) | hand |
| `<lang>.misspellings` | accent-less spellings that are not words (`perche` → `perché`) | `tools/data/find-accent-misspellings.py`, reviewed |
| `<lang>.supplement` | common words missing from the AOSP list (`uomo`, `überhaupt`) | `tools/data/find-supplement.py`, reviewed |
| `<lang>.boost` | raised frequencies of everyday words (`penso`, `voglio`) | `tools/data/find-boosts.py`, reviewed |
| `<lang>.paradigms`, `<lang>.paradigm-skip` | regular verb endings, irregular verbs to leave alone | hand |
| `<lang>.forms` | missing forms of regular verbs (`riuscivi`) | `tools/data/complete-paradigms.py`, review a sample |
| `supplement.stop` | Tatoeba example names and over-used words, kept out of all generated data | hand |
| `../pairs/<lang>.tsv.gz` | starting next-word data (word pairs) | `tools/data/count-pairs.py` |

The generated lists need the sentence corpora (Common Voice, CC0; Tatoeba,
CC BY 2.0 FR; about 400 MB), which are not in the repo. To regenerate the
lists and the test lists of unknown real words (`build/data/oov-*.txt`):

```sh
tools/data/regenerate-lists.sh ~/corpora --fetch    # without --fetch: reuse ~/corpora
tools/data/regenerate-lists.sh ~/corpora es fr      # only these languages
git diff data/rules                            # review before committing
```

Languages: `en` (AOSP en_US), `it`, `de`, `es`, `fr`, `pt_BR`, `pt_PT`,
`nl`, `sv`, `da`, `nb`, `ro`, `fi` (`data/CMakeLists.txt`; the settings list is
`settings/languages.js`). Both Portuguese lists use the same corpora (`pt`).
Adding one: its AOSP list in `data/aosp/`, a `.rules` file, the test
sentences (`tests/data/README.md`), an entry in both places named
above and in `ALL`, `cvdir` and `tatoeba` of `tools/data/regenerate-lists.sh`,
then regenerate and review the lists. The lists of sv, da and nb come
from little corpus text (Common Voice has 0.1–1 MB for them), so their
next-word data is thin. Finnish has many forms per word, and the AOSP
list misses about 10% of the words of Tatoeba text: the supplement (with
`one-source`, as Common Voice has little Finnish) and the `suffix` and
`compound` rules bring that down to about 3%.
