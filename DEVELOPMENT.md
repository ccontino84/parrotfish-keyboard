<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Parrotfish development

## Layout

| Path | What |
|---|---|
| `engine/` | the typing engine: plain C++17, no Qt, so it builds and tests on the PC |
| `engine/tools/pf-query.cpp` | try the corrector by hand |
| `plugin/` | QML module `Parrotfish.Keyboard` (Qt 5.6): the engine as the list model Jolla's `PreeditHandler` expects |
| `handler/` | `ParrotfishHandler.qml` and its `.conf` (registered only for our layout's code, PARROTFISH; Jolla's layouts keep XT9 or Hunspell) |
| `layouts/` | the one "Auto (Parrotfish)" layout (`parrotfish.qml`, using our handler through `handler=` in `layouts_parrotfish.conf`; the name sorts it to the top of Jolla's keyboard list); its letter rows are built from `parrotfish/arrangements.js` (QWERTY: Jolla's en keys plus a full-width apostrophe ending the middle row; QWERTZ: Jolla's de keys; AZERTY: Jolla's fr keys with the apostrophe in place of ç, which moves to C's long press; QWERTY (ÅÖÄ): Jolla's fi keys), picked by `/apps/parrotfish-keyboard/layout`; optional key backgrounds in `parrotfish/` |
| `settings/` | our section in Settings → Text input: `settings=` in both `.conf` files points to it, as XT9's does; the typing languages have their own page (`ParrotfishLanguages.qml`; the list comes from the plugin's `Languages`) |
| `cli/` | `parrotfish`, a command that shows what the keyboard is learning (reads `learning.db`) |
| `translations/` | the settings' translations, `parrotfish-keyboard-<lang>.ts` (qsTr; the context is the QML file's name, a second argument tells gendered words apart: `qsTr("Off", "learning")`); new texts: `lupdate settings -ts translations/*.ts`, then translate them (Qt Linguist or by hand). Built with the SDK's Qt 5 `lrelease` (none on the PC: skipped), installed in `/usr/share/translations`, loaded by the plugin |
| `rpm/` | RPM spec; `split-languages.py` puts the experimental languages' dictionaries in their own package |
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
tools/pf diff alpha70 try       # the words whose outcome changed
tools/pf sweep changeCost "6 7 8"   # a parameter's values, ranked
tools/pf failures               # typos left or fixed wrong, sorted by cause
tools/pf check                  # before a release: lint, tests, suite, and
                                # exit 1 if a number got worse than the last run
tools/pf rpm --version 0.1.0~alpha71   # phone RPM (sfdk), copied if configured
```

`pf rpm` builds the dictionaries on the PC and hands them to the RPM's build
in `prebuilt/` (git-ignored, removed afterwards) with a fingerprint of their
inputs; the RPM's build copies them only if its inputs have the same
fingerprint (`data/CMakeLists.txt`), else builds them itself. The files are
byte for byte the same either way; the RPM takes about 40 s instead of
10 minutes. `pf rpm --from-source` builds them in the SDK, as a release build
(no `prebuilt/`) does.

The suite, for all 13 languages: typos in running text with the number row,
uncertain taps and names mixed in; codes with a digit; words of another
language in mixed texts; next words and keystrokes saved; real misspellings
from your own lists. Each language is typed with a typical set of
dictionaries loaded (en, it, de together; fr, es, en; the others with en)
on QWERTY, or QWERTY (ÅÖÄ) for sv, da, nb, fi. `pf table [LABEL]` shows a
run's main numbers, one line per language; `pf compare` marks the cases
one of the runs lacks or failed. Each run stores its wall and CPU time
(per case too); the cases run longest first.

Faster runs and sweeps:

- `pf suite` reuses a case whose fingerprint (the bench binary, with the
  engine linked in, and the contents of every file it reads: dictionaries,
  layout, texts, lists; its options and `--param`s) matches a stored run,
  and runs only the others: after a change to Italian data only the cases
  that load it run again; nothing changed, about 2 s. `--all` runs
  everything. Cases longer than an even share of the work are split by
  texts across the cores (`-k`, `-Z`, joined by the bench's `-J`: the same
  numbers); `--no-shard` doesn't.
- Decision settings (the parameters only `Engine::decide` reads:
  `isDecisionParam` in `engine/src/params.cpp`: the decision costs and thresholds, margins,
  `unknown*`, strength other than Strong) are replayed on the same searches:
  `pf suite --label L --replay changeCost=8 --replay
  unknownBase=11,changeCostShort=15` stores `L` and `L[SETTINGS]`
  for each, the same numbers as runs with `--param`, for about the time of
  one run.
- `pf sweep NAME "V1 V2 ..."`: replayed in one run if NAME is a decision
  parameter; otherwise two stages: every value on the first 300 texts of
  each case (`--quick N`; the test texts are random samples, so the first
  lines are one too), then the `--best K` (2) on the whole suite. Ranked by
  `--score` (default `-wer_after`: metrics averaged over the cases).
- `pf diff A B [CASES]`: the words whose outcome changed between two runs
  (fixed, wrong, left; correct words kept, changed), per case, with counts
  and examples (typed (meant: what A gave / what B gave)). `pf suite`
  keeps each sentence-bench case's words with a typo or a change
  (`sentence-bench -O`, about 0.8 MB a run, next to the run's file).

Why typos stay unfixed: `pf failures [CASES] [--param NAME=VALUE]` reruns
the suite's sentence-bench cases (default `^(typos|missp)-`) with `-C`
(each typo, misspelling and changed word with the engine's candidates and
`Result::reason`) and sorts every typo or misspelling not fixed, or fixed
to another word, by cause: (a) the target is in no loaded dictionary; (b)
it is not among the candidates; (c) a commoner word ranks first; (d) it
ranks first, but the "unknown word" hypothesis is its main rival (the
confidence falls short); (e) it ranks first and a rule keeps it out (the
reason: language not sure, another language's word, the confidence
thresholds against other candidates, digits, a capital after a word...);
(f) another word ranks first for other reasons (taps, context). Per case:
counts, the commonest target words of each cause with an example, the
changed correct words; then a table of causes per case. Settings of one machine are in
`~/.config/parrotfish-dev.ini` (`tools/pf config`): where results go
(outside the repo), where `pf rpm` copies the RPM, the sfdk target, more
test texts of your own for the next-word cases (`[eval]`,
`NAME = LANG:PATH`, one message per line), and lists of real misspellings
for the `missp-NAME` cases (`[misspellings]`, `NAME = LANG:PATH`, below).

Try words: `build/engine/pf-query -l data/layouts/it.layout -d build/data/it.pfd perche citta\'`

Benchmarks:

```sh
build/tests/typo-bench -l data/layouts/de.layout -d build/data/de.pfd [-d build/data/en.pfd] \
    [-o tests/data/unknown-de.txt] [-n 1000] [-P gentleConfidence=0.7]
build/tests/lang-bench -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd [-P switchProb=0.01]
```

Running text, as on the phone (all languages, previous words as context):

```sh
build/tests/sentence-bench -l data/layouts/en.layout -t tests/data/tatoeba-it.txt \
    -d build/data/it.pfd -d build/data/en.pfd -d build/data/de.pfd [-P strength=2]
```

Words keep a final apostrophe only where the word with it is in a loaded
dictionary (Italian po'); closing quotes and possessives the dictionaries
don't have (boys') are typed without it (`tests/bench/typos.h`,
`typedWords`; `-E` prints the words as typed). Each sentence is a new text; `-m it` starts it leaning towards a language as
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
    -X -N 75 -o tests/data/unknown-en.txt [-S slips.log [-M]]
tools/eval/bench-sweep.sh changeCost "5 7 9" ...same arguments...
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

Real misspellings, as people spell (recieve, propio, Vorraussetzung), not
slips of the finger:

```sh
build/tests/sentence-bench -l data/layouts/phone-qwerty.layout -t tests/data/sentences-en.txt -m en \
    -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd -N 75 -r 0 -W en-misspellings.tsv [-A]
```

`-W` reads a list (`misspelling<TAB>correct[<TAB>source<TAB>flags]`, `#`
comments; flag `accents-only` for citta, Strasse) and types the words of the
sentences that have one as one of them, as is: a share `-w` (1) of them, at
most `-c` (3) times each word. A two-word correction (alot for "a lot")
matches two words. `-A` types each pair once instead, after three words of
a sentence. The rows "misspelling" and "missp. accents" and the summary say
how many were fixed, changed to another word (wrong) or left. The lists
are not in the repo (licences); `tools/pf suite` runs the ones named in
`[misspellings]` as `missp-NAME`.

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
(`0.1.0~alpha1`); only CI release builds have the plain version. A version
with a `~` also gets `PARROTFISH_DEVTOOLS=ON` (the spec decides): `pf-query`
and the benchmarks in `/usr/libexec/parrotfish-keyboard/`, with their test
sentences. Release builds leave them out.

Three packages: `parrotfish-keyboard` (no dictionaries), and two noarch
language packages, `parrotfish-latin-languages` and
`parrotfish-latin-languages-experimental` (the languages marked
experimental in `data/rules/*.language`); the spec's file lists come from
`languages.json` (`rpm/split-languages.py`). A language package requires
`parrotfish-keyboard` and `parrotfish-dictionary-format = N`, which the
keyboard provides (the spec's `dictformat`, checked against the engine's
`FormatVersion` by ctest `spec`): a keyboard update needs new language
packages only when the format changes. The keyboard doesn't require a
language package (one file at a time, from the Files app, neither could be
installed first); without one it types without suggestions and the settings
show "No languages installed". The plugin offers only the languages whose
dictionary is installed; a typing language without one is skipped.

`parrotfish candidates` lists the new words not learned yet (texts so far,
uses, words left before the count starts over), `parrotfish learned` the
learned new words (count, and the fadings and words of typing
until one is forgotten if not used again; `--all` adds the dictionary words
whose use was learned); `-l LANGUAGE` for one language. It reads
`learning.db` read-only, with the engine's default `LearnParams`; the
keyboard saves when it closes.

How it runs: `ParrotfishHandler.qml` is Jolla's `PreeditHandler` with our
`PredictionEngine` as its `engine`. When a word starts, the handler passes
the letter keys of the current layout (centres and sizes) and the text
before the cursor; then each change of the word being typed updates the
suggestions. Autocorrect strength is the dconf key
`/apps/parrotfish-keyboard/autocorrect` (`strong`: `normal`, and after a
previous word a valid word one slip away from a far likelier word of the
same language is fixed too, `strongMargin`; `normal`, the default;
`gentle`, `off`); swiping to another layout can be switched off on our layouts:
`/apps/parrotfish-keyboard/swipe` (`normal`, the default; `off` = a drag of
a full keyboard width); a long press on the spacebar opens Jolla's menu of
layouts either way. An empty field leans
towards the language the last text was in:
`/apps/parrotfish-keyboard/last-language` (written by the handler).
`/apps/parrotfish-keyboard/languages` holds the typing languages, codes
separated by spaces (`"en it de"`); empty, the default, means the system
language only (English if there is no dictionary for it); others are added
in the settings. Every enabled language is searched on each keystroke, the
likeliest first, the others only for words that can still compete
(`searchMargin`) (settings page: `settings/ParrotfishLanguages.qml`).
The keys: one layout in Jolla's list per
arrangement (`layouts/layouts_parrotfish.conf`: Auto (QWERTY), Auto
(QWERTZ), Auto (AZERTY), Auto (QWERTY ÅÖÄ), the last as Jolla's Finnish and
Swedish layouts), each `layouts/parrotfish/ParrotfishKeyboard.qml` with its
`arrangementKey` (`layouts/parrotfish*.qml`); any typing language on each.
Jolla's keyboard switches between the enabled ones (spacebar long press,
swipe); they are switched on in Jolla's own Keyboards page, as any other
(the diagnostics show Jolla's enabled and active layouts). The look of the
keys of our layouts (`layouts/parrotfish/`):
`/apps/parrotfish-keyboard/key-fill` (`none` = the default, `flat`,
`shaded`) and `/apps/parrotfish-keyboard/key-border` (`none` = the default;
`line`; `raised`, a rim lit from above drawn by a shader), in any
combination; both `none`, the default, is Jolla's look. Letter keys stay fainter than
the spacebar. `/apps/parrotfish-keyboard/key-gap`: the space around each
key background, across and down, in percent of the row height (0-10, `5` =
the default; also for Jolla's backgrounds of `?123`, the spacebar and
Enter). `/apps/parrotfish-keyboard/key-height` (portrait, 80-100, `85` =
the default) and `key-height-landscape` (`100` = the default) shorten every
row of our layout, in percent of Jolla's height (`layouts/parrotfish.qml`
replaces `keyHeight` as Jolla's `updateSizes()` sets it).
`/apps/parrotfish-keyboard/number-row` (`true` = the default, `false`) adds
a row of digits above the letters (`withNumberRow()` in
`layouts/parrotfish/arrangements.js`): the digit row of the first symbol
page becomes that row; the first symbol page is then the second page's
first two rows over the first page's other two, and the second page holds
rare signs (fractions, quotes, maths, ©).
`/apps/parrotfish-keyboard/number-row-height`: its height in percent of the
other rows (`75` = the default, "Smaller", with smaller digits; `100`;
any value from 40 works). `/apps/parrotfish-keyboard/sticky` (`true` = the
default, `false` = Jolla's rows: `withoutSticky()`) and `sticky-side`
(`right`, `left`): the sticky key, which keeps the sign last picked from its
long press (`sticky-key`). `/apps/parrotfish-keyboard/long-press-hint`
(`dot` = the default, `off`): a dot on keys with a long-press menu. The
settings section (`settings/ParrotfishSettings.qml`) opens two pages,
Typing and Look & feel; Look & feel opens the keyboard, with a field to try it in that it doesn't learn from
(`settings/KeyboardPreview.qml`: its `__inputMethodExtensions` carry
`parrotfishNoLearning`, which the handler passes to the plugin's
`learnPaused`). Below the two rows, Reset unsets every key under
`/apps/parrotfish-keyboard/`, as on a new installation (after a remorse
timer; nothing learned is touched, nor the debug file). A double tap on
the version opens the diagnostics: version, system, settings and learning
counts, no words, to copy, or sent by Report a bug, which opens a new issue
in the browser (the form in `.github/ISSUE_TEMPLATE/bug_report.yml`) with
them filled in (`KeyboardInfo` in the
plugin, which also lists the settings' keys and resets them). With `~/.local/share/parrotfish-keyboard/debug` present, the plugin logs
timings (never the typed text) to the journal, and appends to
`~/.local/share/parrotfish-keyboard/slips.log` the kind of slip behind every
autocorrection kept or undone, every correction picked from the bar and
every word changed by hand (Backspace back into a committed word):
`kept key k l` (typed k, meant l), `kept space missing` and `kept space
key n` (two words typed as one), `undone key j h`, `undone late key g h`
(an autocorrection changed by hand after more typing; it also has a
`kept` line), `redone key j h` (an undo taken back: Backspace into the
restored word, then Space, a pick or retyping the correction; it is
neither picked, kept nor manual), `manual key r e` (a word the engine left
alone, fixed by hand; a quite different word is not logged), `picked swap`,
`strong kept key d s margin=9.0` (a valid word fixed by Strong, with its
margin in nats; also `strong undone`), `missing`,
`missing-double`, `extra`, `extra-double`, `accent`, `other`. Letter pairs
only, never words; it is the real tap data for the key model (which slips
are common on the phone). A header line names the settings the lines below
come from, written again whenever they change, each day and after a
restart: `# 2026-10-29 0.1.0~alpha51 QWERTY portrait height=90 normal
pitch=1.67 learning=on` (date, version, arrangement, orientation, key
height %, strength, rows apart in key widths as measured, learning). `words 50` lines count the words typed letter by
letter under that header (in batches of 50, and the rest when the settings
change; up to 49 are lost at a restart), so slips can be compared per word.

The undo memory (typed words whose autocorrection was undone; two undos
stop it, in text of the language the text was in when it was corrected)
is kept between sessions in
`~/.local/share/parrotfish-keyboard/learning.db` (SQLite, the system's
`sqlite-libs`), whether or not the debug file is there: words, so no
debug data. Every table has a cap (undos: the 1,000 used last) and freed
space is given back. Deleting the file, with the keyboard closed (or
before `systemctl --user restart maliit-server`), forgets everything.

Plugin test on the PC: install to a staging dir (`DESTDIR=… cmake --install
build`) and load it in `qmlscene-qt5` with `QML2_IMPORT_PATH` and
`PARROTFISH_DATA_DIR` pointing there.

## Continuous integration and releases

`.github/workflows/checks.yml` runs on every push and pull request:
`tools/qt56-scan.sh`, `reuse lint`, then the build and `ctest` in an
Ubuntu 24.04 container (Qt 5.15; the QML tests draw in software, no
OpenGL). It keeps the dictionaries it built as the artifact
`dictionaries`.

Making a release:

1. On the release branch (`dev/<major.minor>`): one commit per feature, each
   with its `CHANGELOG.md` bullet under `## <version> (unreleased)`. Then a
   release commit: `Version:` in `rpm/parrotfish-keyboard.spec` with no `~`
   suffix, and the entry's heading dated, `## <version> (date)`. Push the
   branch.
2. On GitHub: **Actions > Release > Run workflow**, from the release branch.
   `.github/workflows/release.yml` checks that the version has no `~`, that
   `v<Version>` doesn't exist, that `CHANGELOG.md` has its entry and that
   `main` can fast-forward to the commit. It runs the checks, then builds
   the RPM for aarch64 and armv7hl against the Sailfish OS 5.1.0.11 SDK
   targets with the checks' dictionaries in `prebuilt/`. Finally it creates
   the release `v<Version>` for that commit, with both RPMs attached. The
   release text is the changelog entry (`ci/release-notes.sh <version>`)
   followed by GitHub's generated notes. By default the release is a draft.
3. If it fails: fix it on the branch, delete the draft if there is one,
   push and run it again. `main` hasn't changed.
4. If it works: fast-forward `main` (`git push origin dev/<major.minor>:main`),
   try the draft's RPMs on a phone, then publish the draft. Publishing
   creates the tag `v<Version>` on that commit.

The RPM builds use the community Sailfish SDK images
([coderus/sailfishos-platform-sdk](https://hub.docker.com/r/coderus/sailfishos-platform-sdk))
through `ci/build-rpm.sh`, which also runs locally with Docker:

```sh
ci/build-rpm.sh aarch64      # or armv7hl; RPM in out/ (the image is about 3 GB)
```

With dictionaries in `prebuilt/` (copy `build/data/*.pfd` and
`build/data/inputs.sha256` there), the RPM's build copies them instead of
building them under emulation.

## Dictionary data

`tools/data/build-dict.py` turns an AOSP wordlist into a memory-mapped `.pfd`
file (format: `engine/include/parrotfish/dictformat.h`). Besides the words,
each file holds the language's letter model (section `LTRS`, 0.2–0.4 MB): a
character 4-gram of the list's words without accents, so that a word missing
from the list that looks like the language (a name) is left alone more often
than random letters (`EngineParams::unknownLetterWeight`). Long letters one
change from a word get less of that benefit, the longer the less
(`unknownLongLetterCost`): those are mostly the word misspelled (recieve,
goverment), not a new word. It reads, from
`data/rules/`:

| File | Content | Made by |
|---|---|---|
| `<lang>.rules` | spelling rules (`ae ä`, `e' è final`, `ş ș`); settings for the generators: `paradigm`/`paradigm-plain` (real verb forms that are not accent misspellings), `only-marks`, `no-misspellings`, `capitalised-nouns`, `min-freq` (drop the padding of a list; ro), `one-source` (a supplement word needs only N sightings in both corpora together; fi); word forms the list lacks, for the engine (`Dictionary::isWordForm`; fi): `suffix ENDING... [after=LETTERS] [drop=LETTERS]` (endings on a word: kännykkä → kännykkänsä; `drop`: talon → taloni) and `compound MIN PARTS` (up to PARTS words of at least MIN letters, and hyphenated parts). Such forms stand when typed (never autocorrected, and count for their language) but are never suggested | hand |
| `<lang>.keep` | plain spellings that are words of their own, never marked as misspellings (pt `forca`, ro `frumoasa`) | hand |
| `<lang>.misspellings` | accent-less spellings that are not words (`perche` → `perché`) | `tools/data/find-accent-misspellings.py`, reviewed |
| `<lang>.supplement` | common words missing from the AOSP list (`uomo`, `überhaupt`) | `tools/data/find-supplement.py`, reviewed |
| `<lang>.boost` | everyday frequencies: raised for words common in Tatoeba (`penso`, `I'm`), lowered for written-text function words (`in`, `of`); rank mapping, see the script | `tools/data/find-boosts.py`, reviewed |
| `<lang>.paradigms`, `<lang>.paradigm-skip` | regular verb endings, irregular verbs to leave alone | hand |
| `<lang>.forms` | missing forms of regular verbs (`riuscivi`) | `tools/data/complete-paradigms.py`, review a sample |
| `supplement.stop` | Tatoeba example names and over-used words, kept out of all generated data | hand |
| `../pairs/<lang>.tsv.gz` | starting next-word data (word pairs) | `tools/data/count-pairs.py` |

The generated lists need the sentence corpora (Common Voice, CC0; Tatoeba,
CC BY 2.0 FR; about 900 MB), which are not in the repo: keep them in a
`corpora` folder next to the repo (`../corpora`). To regenerate the lists
and the test lists of unknown real words (`tests/data/unknown-*.txt`):

```sh
tools/data/regenerate-lists.sh ../corpora --fetch   # without --fetch: reuse ../corpora
tools/data/regenerate-lists.sh ../corpora es fr     # only these languages
git diff data/rules                            # review before committing
```

Languages: `en` (AOSP en_US), `it`, `de`, `es`, `fr`, `pt_BR`, `pt_PT`,
`nl`, `sv`, `da`, `nb`, `ro`, `fi` (`data/CMakeLists.txt`). Both Portuguese
lists use the same corpora (`pt`). What the keyboard and the settings know
of each language (its name, Experimental, the accented letters of the
long-press menus, its punctuation signs, the usual layout and the system
languages it stands for) is in `data/rules/LANG.language`
(`tools/data/language-list.py` documents the lines), installed as one
`languages.json` and read by the plugin's `Languages` (a QML singleton for
the layout and the settings). Adding one: its AOSP list in `data/aosp/`, a
`.rules` and a `.language` file, the test sentences
(`tests/data/README.md`), an entry in `data/CMakeLists.txt` and in `ALL`,
`cvdir` and `tatoeba` of `tools/data/regenerate-lists.sh`,
then regenerate and review the lists. The lists of sv, da and nb come
from little corpus text (Common Voice has 0.1–1 MB for them), so their
next-word data is thin. Finnish has many forms per word, and the AOSP
list misses about 10% of the words of Tatoeba text: the supplement (with
`one-source`, as Common Voice has little Finnish) and the `suffix` and
`compound` rules bring that down to about 3%.
