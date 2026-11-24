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
| `engine/TUNING.md` | why the engine's settings have their values |
| `docs/prior-art.md` | the published method behind each engine feature |
| `docs/adding-a-language.md` | a contributor's guide to adding a typing language |
| `plugin/` | QML module `Parrotfish.Keyboard` (Qt 5.6): the engine as the list model Jolla's `PreeditHandler` expects; the colour emoji images (`image://parrotfish-emoji`); in `emoji/`, the emoji code in plain JavaScript, `EmojiUtil` (code points, recent emoji, variant picks) and `Emoticons` (`:-)` → 🙂) |
| `handler/` | `ParrotfishHandler.qml` and its `.conf` (registered only for our layout's code, PARROTFISH; Jolla's layouts keep XT9 or Hunspell) |
| `layouts/` | the "Auto" layouts, one file per arrangement (`parrotfish.qml` for QWERTY, `parrotfish_qwertz.qml`, `parrotfish_azerty.qml`, `parrotfish_jcuken.qml`, `parrotfish_greek.qml`: `parrotfish/ParrotfishKeyboard.qml` with its `arrangementKey`), using our handler through `handler=` in `layouts_parrotfish.conf` (the name sorts them to the top of Jolla's keyboard list); their letter rows are built from `parrotfish/arrangements.js` (QWERTY: Jolla's en keys plus a full-width apostrophe ending the middle row; QWERTZ: Jolla's de keys; AZERTY: Jolla's fr keys with the apostrophe in place of ç, which moves to C's long press); the numpad (`numpad` there) replaces the ?123 page's rows when `/apps/parrotfish-keyboard/numpad` is on, not in split landscape; the language last typed is remembered per layout (`last-language`, `last-language-azerty`, ...); the language keys (the typing language's `extra-keys`) placed by `placeLanguageKeys` there; optional key backgrounds in `parrotfish/`; the emoji panel in `parrotfish/emoji/` (`EmojiPanel.qml`, with the emoji list `emojidata.js` from `tools/data/emoji-list.py`; it gets its state through properties and reports through signals, so any layout can host it); other scripts (`parrotfish_jcuken.qml`, `parrotfish_greek.qml`): Jolla's ru.qml/uk.qml and el.qml rows as they are (`jcuken`, `greek` in `arrangements.js`, `script` Cyrl/Grek: `PredictionEngine.script`: the plugin loads one language of that script, the keys' (the switch key's choice), from its typing languages or, if none is chosen, every installed one), ЙЦУКЕН's rows by language (`languages`) with a globe key cycling through them when both Russian and Ukrainian are typing languages or installed (`SwitchKey.qml`, dconf `keys-jcuken`; no automatic switching), Greek's tonos dead key (`DeadLetterKey.qml`); the sticky key is for the Latin layouts only |
| `voice/` | voice input (aarch64 builds): `helper/` (`parrotfish-voice`, recognition in its own process: microphone, Silero VAD, Parakeet through sherpa-onnx's C API, phrase by phrase; a line protocol on stdin/stdout, described in `main.cpp`; no network, through a seccomp filter), `licenses/`; `fetch-deps.sh` downloads the prebuilt aarch64 libraries into `deps/` (not in the repository) |
| `downloader/` | `parrotfish-download`, the only program that uses the network, started by the settings: the speech model (pinned revision) and the dictionaries (as `languages.json` lists them); SHA-256 checked, resumable; a line protocol on stdout, described in `main.cpp`; test `downloader` (a local HTTP server); test `no-network` checks that the plugin and the keyboard's QML don't use the network |
| `plugin/src/voice*` | voice on the keyboard's side: `VoiceInput` (the helper process, QML singleton `Voice`), `VoiceText` (the text, its edits and undo; unit test `voicetext`), `VoiceModel` (the download through `DownloadJob`, singleton `VoiceModel`); the panel and the space key's menu in `layouts/parrotfish/voice/` |
| `settings/` | our section in Settings → Text input: `settings=` in both `.conf` files points to it, as XT9's does; the typing languages have their own page (`ParrotfishLanguages.qml`: the list and each dictionary's state from the plugin's `Languages`, the downloads through `LanguageDownloads`) |
| `cli/` | `parrotfish`, a command that shows what the keyboard is learning (reads `learning.db`) |
| `translations/` | the settings' and the voice panel's translations, `parrotfish-keyboard-<lang>.ts` (qsTr; the context is the QML file's name, a second argument tells gendered words apart: `qsTr("Off", "learning")`); new texts: `lupdate -no-obsolete -locations none settings handler layouts -ts translations/*.ts`, then translate them (Qt Linguist or by hand) and put back the SPDX header lupdate drops. Built with the SDK's Qt 5 `lrelease` (none on the PC: skipped), installed in `/usr/share/translations`, loaded by the plugin |
| `rpm/` | RPM spec (also used by SailfishOS:Chum, below) |
| `tools/split-languages.py` | the RPM's file lists: each dictionary to its language package |
| `tools/pf` | the developer front door (below): tests, lint, benchmark suite, comparisons, RPM |
| `tools/data/` | Python tools that build and maintain the dictionary data |
| `tools/eval/` | benchmark helpers: parameter sweeps, mixed-language texts |
| `tools/qt56-scan.sh` | flags QML and JavaScript that the phone's Qt 5.6 rejects |
| `data/aosp/` | AOSP wordlists (Apache-2.0), unchanged |
| `data/rules/` | per-language rules and reviewed generated lists (below) |
| `tests/data/` | held-out test sentences per language (`README.md` there) |
| `data/layouts/` | the keys of Parrotfish's four arrangements as on the phone, for the PC tests and benchmarks (`pitch`: keys taller than wide; QWERTY logged on a phone, the others by the same geometry); the phone gets `phone-qwerty.layout`, for the moment before the handler passes the real keys |
| `tests/unit/` | unit tests (ctest) |
| `tests/qml/` | the handler on the PC with the plugin, Sailfish OS's modules replaced by stand-ins (ctest `qml`, needs `qmltestrunner`) |
| `tests/settings/` | every settings page on the PC with the plugin and stand-ins for Silica; any QML warning fails it (ctest `settings`, needs `qmltestrunner`, `dbus-run-session` and `dconf`) |
| `tests/bench/` | accuracy benchmarks: typos and autocorrect in running text (`sentence-bench`), next words (`nextword-bench`), single words (`typo-bench`), mixed languages (`lang-bench`) |

## Comments

Comments say what the code can't: a non-obvious reason (Qt 5.6 draws emoji
in black and white; apps delete one code point per Backspace), a trap, or
what a function takes and returns, in a line or two. They don't describe
what the user sees (that is the README and CHANGELOG), repeat what another
comment or this file says, give the reasons behind a design decision, or
tell how a bug was found. Files based on Jolla's list their changes in the
header, briefly. Examples are in English.

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
tools/pf compare before try    # two runs side by side, better/WORSE marked
tools/pf diff before try       # the words whose outcome changed
tools/pf sweep changeCost "6 7 8"   # a parameter's values, ranked
tools/pf failures               # typos left or fixed wrong, sorted by cause
tools/pf check                  # before a release: lint, tests, suite, and
                                # exit 1 if a number got worse than the last run
tools/pf rpm --version 1.1.0~alpha1    # phone RPM (sfdk), copied if configured
```

The suite (Normal, as the phone's default; `--param strength=3` for Strong,
`strength=1` for Gentle): typos in running text per language (`typos-`),
typos that make another real word (`realword-`, `sentence-bench -R`: fixed
only at Strong), codes and other languages' words left alone (`codes-`,
`mixed-`), real misspellings (`missp-`, lists outside the repo) and next
words (`nextword-`). The 1.0 baselines: `1.0-baseline`, `-strong`,
`-gentle`.

`pf rpm` builds the dictionaries on the PC and hands them to the RPM's build
in `prebuilt/` (git-ignored, removed afterwards) with a fingerprint of their
inputs; the RPM's build copies them only if its inputs have the same
fingerprint (`data/CMakeLists.txt`), else builds them itself. The files are
byte for byte the same either way; the RPM takes about 40 s instead of
10 minutes. `pf rpm --from-source` builds them in the SDK, as a release build
(no `prebuilt/`) does.

The suite, for all 20 languages: typos in running text with the number row,
uncertain taps and names mixed in; codes with a digit; words of another
language in mixed texts; next words and keystrokes saved; real misspellings
from your own lists. Each language is typed with a typical set of
dictionaries loaded (en, it, de together; fr, es, en; the others with en)
on QWERTY; sv, da, nb and fi on `phone-nordic.layout` (QWERTY with å ö ä
where their language keys put them), cs, sl and hr on QWERTZ; ru and uk
together on `phone-jcuken.layout` (Ukrainian: `phone-jcuken-uk.layout`, its
own rows), el alone on `phone-greek.layout` (a layout loads its script's
languages only). `pf table [LABEL]` shows a
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
`parrotfish-dev.ini` next to the checkout, else `~/.config/parrotfish-dev.ini`
(or `$PF_CONFIG`; `tools/pf config` shows the one in use): where results go
(outside the repo), where `pf rpm` copies the RPM, the sfdk target, more
test texts of your own for the next-word cases (`[eval]`,
`NAME = LANG:PATH`, one message per line), and lists of real misspellings
for the `missp-NAME` cases (`[misspellings]`, `NAME = LANG:PATH`, below).

Try words: `build/engine/pf-query -l data/layouts/phone-qwerty.layout -d build/data/en.pfd teh wrk helo`

Benchmarks:

```sh
build/tests/typo-bench -l data/layouts/phone-qwertz.layout -d build/data/de.pfd [-d build/data/en.pfd] \
    [-o tests/data/unknown-de.txt] [-n 1000] [-P gentleConfidence=0.7]
build/tests/lang-bench -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd [-P switchProb=0.01]
```

Running text, as on the phone (all languages, previous words as context):

```sh
build/tests/sentence-bench -l data/layouts/phone-qwerty.layout -t tests/data/tatoeba-en.txt \
    -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd [-P strength=2]
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
phone does). `-L` adds the results by typed length (2, 3–4, 5+
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
voice/fetch-deps.sh        # once: voice input's prebuilt libraries (aarch64)
sfdk config target=SailfishOS-5.1.0.11-aarch64
sfdk build                 # RPM in RPMS/
```

Local builds carry a pre-release suffix in the spec's `Version`
(`0.1.0~alpha1`); only CI release builds have the plain version. A version
with a `~` also gets `PARROTFISH_DEVTOOLS=ON` (the spec decides): `pf-query`
and the benchmarks in `/usr/libexec/parrotfish-keyboard/`, with their test
sentences. Release builds leave them out.

Five packages: `parrotfish-keyboard` (no dictionaries), and four noarch
language packages named by script, `parrotfish-keyboard-languages-latin`,
`parrotfish-keyboard-languages-latin-experimental` (the Latin languages
marked experimental in `data/rules/*.language`), `-languages-cyrillic` and
`-languages-greek` (named after the keyboard, so that
installers taking a release's packages together, such as Harpoon, accept
them; `parrotfish-languages-*` until 1.2, which they obsolete); the spec's file lists come from
`languages.json` (`tools/split-languages.py`, which also writes each
dictionary's record, `CODE.pfd.sha256`). On aarch64 the keyboard package
also holds voice input: the helper, sherpa-onnx and onnxruntime (private
copies in `/usr/lib64/parrotfish-keyboard-voice`, neither provided nor
required) and Silero VAD; it obsoletes the voice package of 1.3. The
keyboard shows the microphone only once the model is downloaded
(`~/.local/share/parrotfish-keyboard/voice/`).

Dictionaries come from a language package or from the settings, which
download them into `~/.local/share/parrotfish-keyboard/dictionaries/`
(`CODE.pfd` and its record, written last). `Languages` (plugin) knows
every language of `languages.json` and where its dictionary is; the
engine tries this release's file first (the record matches the pinned
hash), a download before a package's, and the next one if one is refused.
The settings tell the keyboard about a download or a delete through the
dconf key `dictionaries-changed`. With no dictionary, or only refused
ones, the empty suggestion bar says so (`PredictionEngine`'s
`dictionaryProblem`). A language package requires
`parrotfish-keyboard` and `parrotfish-dictionary-format = N`, which the
keyboard provides (the spec's `dictformat`, checked against the engine's
`FormatVersion` by ctest `spec`). The keyboard doesn't require a
language package (one file at a time, from the Files app, neither could be
installed first).

### Publishing dictionaries

The dictionaries build byte for byte the same on every machine. Each one
the settings can download is pinned in `data/dictionaries.json`: the
GitHub release that holds it (`languages-N`, never the latest release)
and the SHA-256 of the compressed and the unpacked file; `languages.json`
gets them as each language's `download`. Test `dictionaries-pinned` fails
when a built dictionary isn't the pinned one. Then, before the next
keyboard release: **Actions > Languages > Run workflow** with the next tag
(`.github/workflows/languages.yml`): it builds the dictionaries, publishes
the changed and new ones as that release with the licence texts, and
keeps the updated `data/dictionaries.json` as the artifact `pinned`;
commit it. Unchanged dictionaries keep their release, so a keyboard
update downloads nothing for them. The same by hand:
`tools/data/languages-release.py prepare build/data data/dictionaries.json
languages-N out/`, then `gh release create languages-N out/* --latest=false`.

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
   the RPMs for aarch64 and armv7hl against the Sailfish OS 5.1.0.11 SDK
   targets with the checks' dictionaries in `prebuilt/`. Finally it creates
   the release `v<Version>` for that commit, with the two
   `parrotfish-keyboard` RPMs and the four noarch
   language packages attached
   (the aarch64 build's copies; every package is checked with `rpm -K`
   before it is attached). The
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

For aarch64 it runs `voice/fetch-deps.sh` first.

With dictionaries in `prebuilt/` (copy `build/data/*.pfd` and
`build/data/inputs.sha256` there), the RPM's build copies them instead of
building them under emulation.

## Packaging for SailfishOS:Chum

Anyone with an account on the Sailfish OS build service can package
Parrotfish for [SailfishOS:Chum](https://github.com/sailfishos-chum/main);
the spec is ready for it. The package's `_service` points `tar_git` at this
repository, pinned to a release tag (`v<Version>`) or its commit:

```xml
<services><service name="tar_git">
  <param name="url">https://github.com/ccontino84/parrotfish-keyboard.git</param>
  <param name="branch">main</param>
  <param name="revision">v1.0.0</param>
</service></services>
```

- `tar_git` takes the version from the tag (dropping the `v`) and leaves
  `rpm/` out of the sources: nothing in the build reads `rpm/`
  (`tools/split-languages.py` is outside it for that reason). The test
  builds' developer tools stay out, as the version has no `~`.
- The dictionaries are built from `data/` during the build (no `prebuilt/`),
  so it takes longer than the RPMs from GitHub; nothing is downloaded.
- No voice input: its prebuilt libraries can't be downloaded during the
  build (the spec's `voice` is 0 with `_chum`).
- One spec, three packages. Each `%description` ends with Chum's metadata,
  so the keyboard and both language packages show up in the Chum app;
  a language package installs the keyboard, which needs no language
  package (as above).
- Tested on Sailfish OS 5.1 and 5.2 (aarch64; armv7hl builds, untested).
  Older targets can be disabled in the package's meta.
- After each release, the package maintainer moves `revision` to the new
  tag.

## Dictionary data

Sources (THIRD-PARTY.md has their licences):

| Source | Taken from | Version |
|---|---|---|
| AOSP LatinIME wordlists (en_US, it, de; es, fr, pt_BR, pt_PT, nl, sv, da, nb, ro; fi; pl, cs, hr; ru, el) | `android.googlesource.com/platform/packages/inputmethods/LatinIME`, `dictionaries/` | commit `127336e9f29d69607eab55982324b210279ae8c5` (fetched 2026-10-08, the second group 2026-10-26, fi 2026-10-28, pl, cs and hr 2026-11-10, ru and el 2026-11-22; list versions 44–54, from 2014) |
| Leipzig Corpora Collection (the uk wordlist, `data/leipzig/`, CC BY; `tools/data/leipzig-wordlist.py`; AOSP has no uk list) | `downloads.wortschatz-leipzig.de/corpora/`: `ukr_news_{2020,2022,2023,2024}_1M`, `ukr_newscrawl_2011_1M` (`tar.gz`), with the Russian AOSP list as `--foreign` and frequency scale | fetched 2026-11-22 (ukr_news_2024 built 2025-02-13) |
| Sloleks Morphological Lexicon of Slovene (the sl wordlist, `data/sloleks/`, CC BY-SA 4.0; `tools/data/sloleks-wordlist.py`; the AOSP sl list has 60k words and missed 7–10% of Slovenian text) | `hdl.handle.net/11356/2080` (CLARIN.SI), `Sloleks.3.1.zip` | 3.1 (2026-02), fetched 2026-11-10 |
| Common Voice sentences | `github.com/common-voice/common-voice`, `server/data/<lang>/` | en, it, de: commit `a28a4cbf9188dafac0cb266f90bc9938f8418564` (2026-10-08); es, fr, pt, nl, sv-SE, da, nb-NO, ro, fi: commit `4c9c6a9c9b0561e029f1d65448edaf62947e12ec` (2026-10-26, fi 2026-10-28); pl, cs, sl, hr: commit `b24381ce7136f2524a1128cb605b49111f79dbea` (2026-11-09); ru, uk, el: the same commit (2026-11-22) |
| Tatoeba sentences | `downloads.tatoeba.org/exports/per_language/<lang>/` | eng, ita, deu: 2026-10-08; spa, fra, por, nld, swe, dan, nob, ron: 2026-10-26; fin: 2026-10-28; pol, ces, slv, hrv: 2026-11-09; rus, ukr, ell: 2026-11-22 |
| Jolla keyboard | Sailfish OS 5.2.0.17 RPMs `jolla-keyboard` (`PreeditHandler.qml`) and `jolla-keyboard-layout-western` (uk.qml, el.qml too); ru.qml from Sailfish OS 5.1.0.11 | 0.10.4 |

The counts come from `tools/data/count-words.py` and `count-pairs.py`; the
derived lists from `find-boosts.py`, `find-supplement.py` and
`find-respellings.py`. The test sentences (`tests/data/`, up to 2000 per
language, random) are left out of all counts. Jolla's layouts, as adapted:
QWERTY gets a full-width apostrophe key at the end of the middle row and `"`
on B in the symbol view; QWERTZ unchanged; AZERTY the apostrophe in place of
ç after N, ç on C's long press (QWERTY ÅÖÄ, fi.qml, was dropped in 1.1:
the language keys put å ö ä in its places); all get æ
ă on A, õ œ on O, ș on S, ț on T, and the letters of Jolla's pl, cs and sl
layouts (ą on A, ć č on C, ď đ on D, ę ě on E, ł on L, ń ň on N, ř on R,
ś š on S, ť on T, ů on U, ż ź ž on Z).

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
| `<lang>.rules` | spelling rules (`ae ä`, `e' è final`, `ş ș`); settings for the generators: `paradigm`/`paradigm-plain` (real verb forms that are not accent misspellings), `only-marks`, `no-misspellings`, `capitalised-nouns`, `min-freq` (drop the padding of a list; ro), `one-source` (a supplement word needs only N sightings in both corpora together; fi, sl, hr, el), `own-letters` (letters with a key of their own that Unicode writes as an accented letter, ru `й`, uk `йї`: their base is themselves, so свои is not an accent slip for свой), `letters` (the letters a word may have besides ' and -: other words are left out, el polytonic καὶ, Latin look-alikes), `both-spellings` (a mark written or left out at will, ru `ё`: the spelling without it is added as a word, not skipped as a variant); word forms the list lacks, for the engine (`Dictionary::isWordForm`; fi): `suffix ENDING... [after=LETTERS] [drop=LETTERS]` (endings on a word: kännykkä → kännykkänsä; `drop`: talon → taloni) and `compound MIN PARTS` (up to PARTS words of at least MIN letters, and hyphenated parts). Such forms stand when typed (never autocorrected, and count for their language) but are never suggested | hand |
| `<lang>.lowercase` | abbreviations the list has only in capitals that are often typed in lower case (`OK`, `SMS`): the lowercase spelling is added as a word too, so typed that way it stands (optional) | hand |
| `<lang>.shorthands` | chat shorthands the list lacks in lower case (`idk`, `btw`): added as words that stand when typed but are never suggested (optional) | hand |
| `<lang>.keep` | plain spellings that are words of their own, never marked as misspellings (pt `forca`, ro `frumoasa`); also words the supplement keeps even where another language's text uses them more (de `Tja`, en `wad`), or that are a single letter or another word's accent variant (el `η`) | hand |
| `<lang>.misspellings` | accent-less spellings that are not words (`perche` → `perché`) | `tools/data/find-accent-misspellings.py`, reviewed |
| `<lang>.supplement` | common words missing from the AOSP list (`uomo`, `überhaupt`) | `tools/data/find-supplement.py`, reviewed |
| `<lang>.boost` | everyday frequencies: raised for words common in Tatoeba (`I'm`, `guess`), lowered for written-text function words (`in`, `of`); rank mapping, see the script | `tools/data/find-boosts.py`, reviewed |
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
`nl`, `sv`, `da`, `nb`, `ro`, `fi`, `pl`, `cs`, `sl`, `hr`
(`data/CMakeLists.txt`). Both Portuguese
lists use the same corpora (`pt`). What the keyboard and the settings know
of each language (its name, Experimental, the accented letters of the
long-press menus, its punctuation signs, the usual layout and the system
languages it stands for) is in `data/rules/LANG.language`
(`tools/data/language-list.py` documents the lines), installed as one
`languages.json` and read by the plugin's `Languages` (a QML singleton for
the layout and the settings). Adding one: [docs/adding-a-language.md](docs/adding-a-language.md).
The lists of sv, da and nb come
from little corpus text (Common Voice has 0.1–1 MB for them), so their
next-word data is thin. Finnish has many forms per word, and the AOSP
list misses about 10% of the words of Tatoeba text: the supplement (with
`one-source`, as Common Voice has little Finnish) and the `suffix` and
`compound` rules bring that down to about 3%.
