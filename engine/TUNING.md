<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# How the engine's settings were chosen

`EngineParams` (engine/include/parrotfish/engine.h) says what each setting
does; this file keeps the measurements behind the values, newest first
within each setting. Benches: `tools/pf suite` (13 languages since
alpha90), `sentence-bench`, and informal development text for local
evaluation (not in the repo). "Fixed", "wrong": typos fixed and fixed into
a wrong word, in % of typos; "names changed": unknown real words changed.

## The decision (alpha92)

`changeCost` 7 (3-4 letters 20, 2 letters 6), `wrongCost` 5,
`nearWrongCost` 1.5, Gentle `gentleConfidence` 0.95 with its limits.
Chosen with replay sweeps (`pf suite --replay`), a sign test on the words
whose outcome changed, and informal text. Against alpha91 (13 languages,
real runs): Normal and Strong typos fixed +3.1 points (no language worse),
fixed wrong -0.12, WER -0.61, real misspellings fixed +1.7; Gentle fixed
+4.6, fixed wrong the same, WER -0.9, misspellings +2.5; informal text
fixed +4.1, WER -0.84. Known losses: names and valid forms missing from
the dictionaries changed (it Maira, Vincenzi, fidarvi; es spelling
variants), German and Swedish misspellings in chat text -1.2 / -2.4.
`changeCost` 6-8 trades real misspellings against unknown words; a lower
`wrongCost` fixes more but makes more wrong fixes; without the per-length
costs short common slips are lost (tou: you) or short names changed; without
`nearWrongCost`, misspellings with an inflected neighbour stay (recieve:
received). Before alpha92 Normal and Strong used thresholds on the
confidence by length (0.5; 3-4 letters 0.8; 2 letters 0.7).

Gentle before alpha92: 0.875 (with one unknownBase for all strengths,
alpha86; Gentle, 5 languages x 2 seeds, 2026-11-01): typos fixed 32.5 ->
32.3 %, fixed wrong 0.41 -> 0.26, names changed 0.13 -> 0.16; real
misspellings fixed the same or more (accents it +8.7, fr +3.6 points),
fixed wrong lower (it 2.6 -> 0, de 0.9 -> 0.3). 0.85: more fixed, more
names changed (0.19); 0.9: names as before, 2.4 points fewer fixed. Up to
alpha85 Gentle had its own unknownBase (6, against 8) and 0.5.

## The unknown-word reading

alpha92: calibrated on every word of the 13 typos cases (`pf failures
--all-words`): the log-loss of fix / typed / other fell from 0.785 to 0.400
with `unknownBase` 11.5, `unknownLetterWeight` 0.5 and `unknownOddsScale`
0.6; before, 8 and 0.4 overrated "unknown" about 15 times. Lower letter
weights are worse. `compoundBonus` 8 with the `compound ... unknown` rules
(de nl sv da nb): closed compounds stay, two words typed together are
still split.

The letter model (sentence-bench -L, en/it/de/fr/es, 12 runs each, Normal,
2026-11-01), per letter only -> letter model -> with the shrink of
`unknownLongLetterCost`: 44.4 -> 47.5 -> 51.9 % of typos fixed, 1.12 ->
0.98 -> 1.07 % fixed wrong, 1.23 -> 0.67 -> 0.91 % of unknown words changed
(most of the rest are words without their accents); 3-4 letters: 47.2 ->
45.5 -> 45.5 % fixed, 3.8 -> 1.8 -> 1.8 % changed. Gentle 29.8 -> 34.1 ->
36.4, 0.63 -> 0.48 -> 0.51, 0.40 -> 0.23 -> 0.25. Real misspellings (-W)
fixed: en 41 -> 23 -> 40 %, it 57 -> 32 -> 55, de 51 -> 30 -> 51, fr 57
-> 25 -> 56, es 39 -> 23 -> 37; fixed wrong lower than per letter only.
The gain is in typos with unlikely letters (h3llo, thisbis) and long
typos; short typos that look like words are fixed less (they look like
short names). Weights 0.35-0.45 did as well, 0.2 worse. The shrink from 3
letters on (1 nat) fixes short typos as per letter only, at 1.95 % of 3-4
letter names changed; a cap on what the letter model takes off, at any
length, does worse.

`unknownTwoChangeLength` 9 (conoleanno: compleanno; sentence-bench,
2026-11-01): off -> 9: real misspellings fixed +0.2 to +0.6 points (en, fr,
de, es), fixed wrong and names changed the same; 8 and less: more fixed
wrong (de 1.4 -> 1.6 %).

## A capital after a word

Words stored only in lower case are typed with a capital after a word in
0.3-0.8 % of Tatoeba's (5-6 nats), 3-6 % of news and informal text (3
nats: titles, brands, missed sentence starts), German 4-5 %.
sentence-bench -L, Normal (2026-11-01), `capitalLowerCost` 0 -> 4 (German
2): unknown words changed en 0.47 -> 0.18 %, it 1.37 -> 0.62, fr 1.38 ->
1.02, es 0.83 -> 0.53, de 0.70 -> 0.52; typos fixed 0 to -0.3 points, fixed
wrong 0 to -0.2; next-word keystrokes saved +0.1 to +1.2. Real misspellings
in news text fixed -0.5 to -2.4 points, all capitalised title words (Cheif
Editior). Costs 3 to 5 do about as well; 4 keeps Mr Cant (2 does not);
German at 4: typos fixed -0.7 points. alpha88: 5, with the everyday
frequencies (can't 160 -> 166 made Mr Cant can't again at 4; 4.5 is the
least that keeps it): against 4, real misspellings fixed -0.1 to -0.3
points, names changed es 0.71 -> 0.65 %, slips the same.

## A word of another typing language

`otherLanguageMargin` 12 (sentence-bench, en/it/de, 2026-10-31), against
the hard block up to alpha68 (infinity): 2-3 more typos fixed in 100
(thier is the German surname Thier, tp the abbreviation TP), and no word
of another language changed in mixed texts (tools/eval/mix-sentences.py;
at 8: 0.4 % of single foreign words, un -> in).

## Strong

`strongMargin` 5 (about 150 times; sentence-bench -R, plan step 6).
