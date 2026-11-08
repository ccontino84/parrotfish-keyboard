<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# How the engine's settings were chosen

`EngineParams` (engine/include/parrotfish/engine.h) says what each setting
does. This file says why it has its value: what the value trades, and what
was tried and dropped, so that a change can be measured against the same
things.

## Measuring a change

`tools/pf suite` runs the benchmarks in 13 languages (typos, real-word
typos, real misspellings, mixed-language text, next words) and stores the
run; `tools/pf compare` sets two runs side by side (DEVELOPMENT.md). The
numbers that count: typos fixed, typos fixed into a wrong word, unknown
words (names, rare words) changed, and the error rate after correction.
For a close call, a sign test on the words whose outcome changed.

Every setting below trades the first against the others: a keyboard that
fixes more also changes more words it should have left alone.

## The decision

`changeCost` 7 (3-4 letters 20, 2 letters 6), `wrongCost` 5,
`nearWrongCost` 1.5; Gentle: `gentleConfidence` 0.95.

- `changeCost` 6-8 trades real misspellings fixed against unknown words
  changed; a lower `wrongCost` fixes more and fixes more into a wrong word.
- Without the costs by length, short common slips stay (tou: you) or
  short names are changed.
- Without `nearWrongCost`, a misspelling next to an inflected form stays
  (recieve: received).
- Gentle at 0.85 changes more names; at 0.9 it fixes fewer typos for no
  fewer names changed.
- Known losses: names and valid forms the dictionaries lack.

## The unknown-word reading

`unknownBase` 11.5, `unknownLetterWeight` 0.5, `unknownOddsScale` 0.6:
fitted on every word of the typo benchmarks (how well "fix", "typed" and
"other" are predicted); earlier values overrated "unknown" about 15 times.
The letter model fixes typos with unlikely letters (h3llo, thisbis) and
long typos; short typos that look like words are fixed less, since they
look like short names. `unknownLongLetterCost` brings short typos back.

- `compoundBonus` 8 with the `compound ... unknown` rules (de nl sv da nb):
  closed compounds stay, two words typed together are still split.
  `compoundNearBonus` 3: lower fixes more misspelt compounds; at 0 valid
  inflections are changed.
- `unknownTwoChangeLength` 9 (neccesary: necessary): 8 or less fixes
  more into a wrong word.

## The word before

64 next words stored per word, `pairReach` 12 and `pairSupportRank` 12
(the search adds, and Strong trusts, only the likeliest 12). Against 12
stored: more typos and misspellings fixed and more keystrokes saved in
every language, for bigger dictionaries (format 5) and a slightly slower
search. Weighing each word's next words by how much data it has
(Witten-Bell) was not better than the fixed `pairWeight`.

## A capital after a word

`capitalLowerCost` 5 (German 2). A word stored only in lower case but
typed with a capital after a word is mostly a name, a title or a brand, so
it is left alone. Lower costs change more names (Mr Cant needs at least
4.5); the cost is capitalised misspellings in titles (Cheif Editior).

## A word of another typing language

`otherLanguageMargin` 12: a word of another typing language can be a fix
(thier: their) without words of the other language being changed in mixed
text (at 8: un → in).

## Strong

`strongMargin` 5: a real word is fixed into one about 150 times likelier
after the previous word. Fixes 15-42 % of real-word typos; correct words
changed stay within a few in 10,000. `strongHomeMargin` 3 keeps it from
turning a word of one language into one of the other in mixed text (con →
von, bene → been).

## Speed

On the phone the search takes 1-5 ms for 1-5 letters and up to about 13 ms
at 8. A letter is shown before its search runs, taps faster than the
search share one, and the text before the cursor is read once per change.

## Learning

`newWordTexts` 3: a new word is learned after it is kept in 3 texts (2
learned too many words, typos among them). `learnedWeight` 0.25,
`learnedK` 2000, `learnedPairWeight` 0.5, `learnedPairK` 10: learning
fixes more typos and saves keystrokes in every language tested; fixed into
a wrong word goes up a little with any weights, as the user's own frequent
words compete. Fading keeps the learned data from growing (without it,
results get worse over long use and more typos are learned); it runs when
the keyboard closes.

## Tried and dropped

- Correcting a name typed in lower case into the name: no better than
  leaving it.
- Up and down slips weighed by key height: logged taps showed they are
  rare, so the tap model weighs distances in key widths.
- Leaving words typed with a capital alone at Strong: saves a few names,
  loses German nouns.
- Waiting before searching, or reusing the search when a letter is added:
  the queue already merges fast taps, and reuse means restructuring the
  search for a gain on long words only.
