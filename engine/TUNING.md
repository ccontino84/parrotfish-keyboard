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

`compoundNearBonus` 3 (2026-11-03): with `compoundBonus` 8 also near a
word, misspelt compounds stayed (Bildungsystem, beziehungweise, informal-text
German misspellings fixed 46.7 -> 45.5 % from alpha91 to alpha92). Suite,
against 8 (the same at Normal and Strong): 4: 8 more fixed, nothing
worse; 3: 23 more fixed, sogenanten -> sogenannten counted wrong (a valid
spelling), Danish overvågningskameraet changed; 2: 33 more fixed, 3
worse; 0: 48 more fixed, 21 unknown words changed (inflections:
klasseværelset -> klasseværelser). Gentle: 3 and 4 fix 3-4 more. informal
text at 3: German misspellings fixed 45.5 -> 46.1 %, other languages the
same; the compounds left (Bildungsystem 0.68, vergleichweise 0.86) are
the decision's `changeCost`.

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

## The word before

64 next words per previous word (`count-pairs.py --next`, alpha94; 12
before), `pairReach` 12 and `pairSupportRank` 12 (the search adds, and
Strong's slip rule trusts, only the likeliest 12 as before). Next words at
ranks 13-64 are 15-23 % of held-out pairs, 4-15 % of chat pairs. Suite
against alpha93 (2026-11-03): typos fixed +2.0 to +4.9 points at Normal and
Strong, +3.3 to +6.0 at Gentle, error rate after correction lower in all 13
languages; real misspellings fixed +3.1 to +8.3 (Gentle +5 to +13.4);
keystrokes saved +0.3 to +1.8; fixed wrong -0.2 to +0.2, unknown words
changed up to +0.28 (pt_PT quentinha -> que tinha, fi sinuna -> sinun).
Weighing each word's next words by Witten-Bell trust instead of the fixed
`pairWeight` 0.5 was not better (with `changeCost` 8-10 neither). Cost:
dictionaries +0.2 to +0.5 MB with 4-byte entries (format 5); about 8 µs more per
tap on the PC (next-word bench +8 %, typo bench +3 %).

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

## A name typed in lower case

`nameLowerCost` 0 (off; 2026-11-03): a fix that changes letters into a
capitalised word typed with a lowercase first letter (ertore: errore,
not Ettore). informal development halves, it and en, learning off and on,
costs 2, 3 and 5 against 0: typos fixed -0.2 to +0.1 points, fixed wrong
the same, error rate after correction -0.03 to +0.04: within noise, so
left off; the code was removed (2026-11-04).

## Strong

`strongMargin` 5 (about 150 times; sentence-bench -R, plan step 6).

13 languages (2026-11-04, sentence-bench -R -r 0.1, Tatoeba and sentence
texts): real-word typos fixed 15-42 % (Normal 0), correct words changed
+0.00 to +0.05 points; suite at Strong against Normal: error rate after
correction lower in all 13 (-0.14 to -0.65 points). Mixed-language text
changed +0.02 to +0.24 points: a word of the other language turned into
one of the text's (con → von, bene → been, he → je, go → og), since each
dictionary also lists the other languages' words, rarely.
`strongHomeMargin` 3: both words within 3 nats of their likeliest typing
language, in one language: those changes go (de-it 6 words → 2, un → in
left: Italian has both), single-language results the same (1.5 and 6
give the same). What's left in mixed text is single-language (yell → tell,
ached → aches, bære → være). Tried and dropped: leaving words typed with
a capital alone (saves Rima → Roma, loses German nouns and names typed
with a slip: Nett → Bett, Nary → Mary, 13 against 2).

## Speed

Phone (Jolla Phone, 2.0 and 2.4 GHz cores; plugin debug log while typing,
alpha95): the search takes 1-5 ms for 1-5 letters and up to about 13 ms
at 8 letters. Back to back it runs about 1.5 times slower than on the PC;
with half a second between taps, 8-10 times slower (the clock drops
between taps).

- alpha95: a letter is shown before its search runs (the search is
  queued); taps that come faster than the search share one search.
- alpha96: the text before the cursor is read once per change. Before
  that it was read up to four times per Space (the committed word, the
  app's text, its cursor, the next word's first letter), about 0.5 ms on
  the PC per read at 1,000 characters.
- Not done: waiting a moment before searching. Taps are 150-300 ms apart,
  Space needs the result at once, and the queue already merges fast taps.
- Not done (2026-11-03): reusing the search when a letter is added. Which
  candidates the search keeps depends on the whole input (the best score
  so far, slips at the end, the unknown-word reading), so reuse means
  restructuring the search: an estimated few days of work for 2-5 times
  faster on long words only, while the slowest logged letter is about
  13 ms. To revisit if learning makes the search slower or phone logs
  pass about 30 ms.
- Planned with learning: the results for the last few prefixes are kept,
  so Backspace shows them without searching again.

## Learning

`newWordTexts` 2 (user, 2026-11-05: 3 was too slow on the phone). Long
run against 3 (a long simulated run, en 610k and it 600k words kept,
typos left by a typist who never fixes one by hand, so the cost is
overstated): 2-8x the new words learned, learned typos en 21-35 against
2-8, it 10-16 against 0-3; error rate after correction +0.04 to +0.16
points, worse by sign test from 50k words on (en 500k-600k: 34 better,
113 worse). Accepted: two separate texts within 5,000 words, typos
autocorrect or the user fixes never count, fading drops what isn't reused
(about 70k words), and Learned words removes any.

`learnedWeight` 0.25, `learnedK` 2000, `learnedPairWeight` 0.5,
`learnedPairK` 10, `learnedReach` 0 (2026-11-03). Bench learning mode
(`-Q`): informal text of one language as one user's (many people's posts:
a community's style more than one person's), the first 500 posts typed
and learned, the next 500 measured while learning goes on; 20 unknown
words of the language's list recurring (`-o`, `-u 0.02`) as the user's
names; a quarter of the engine's mistakes kept unnoticed (`-x 0.25`).
Test halves, against learning off: typos fixed +2.4 to +3.6 points, fixed
wrong +0.1 to +0.5, error rate after correction -0.5 to -0.9 points,
next-word keystrokes saved +1.5 to +2.5 (en it de fr es nl pt_BR); sign
test on the words whose outcome changed: better in every language (en
206 better, 59 worse; de 122, 61). Learned typos: 1 in 160 learned words
(de "ser"). Fixed wrong goes up with any weights (0.1 to 0.5, pair
weight 0 to 0.75, `learnedK` 5000: within noise of each other): the
user's own frequent words compete (it: Ettore, often in the learned
text, now keeps ertore from errore). Without the dictionary words the
user makes likelier in the search (`learnedReach` 0): keystrokes saved
-0.3 points (500 words; 100: no gain). Speed (`pf-time`, en it de, 2,000
English sentences learned, PC): per letter 209 -> 273 µs with
`learnedReach` 500, 218 µs with 0; next words 78 -> 116 µs. Hence 0: the
learned new words are still searched (`learnedReach` removed 2026-11-04;
`keyHeightShare`, tap distances up and down in key heights, too: 0 since
slips.log showed vertical slips are rare). Phone (alpha99, `pf-time -p 300`,
the same texts): per letter 3.8 -> 4.0 ms mean, 17.6 -> 18.9 ms at most;
first letter 1.6 -> 2.6 ms (the next words of the new previous word);
next words 1.3 -> 1.8 ms.

Over a long use (2026-11-04, `sentence-bench -Q -I`, Tatoeba streams, 1M
words en/it, 0.5M de/fr, `-x 0.25`): the learned data level off after
about 100k words (5-8k words, 20-25k pairs, the caps never reached);
every stretch better than learning off (en +4 points fixed, it +5-6);
learned typos at 0-11 at any time. Without fading: worse after 500k words
and 4x the learned typos; `fadeEvery` 5000 or 20000, `dropBelow` 1,
`newWordWindow` 10000: worse or even. Defaults kept. Fading rewrites a
whole language in the store (22-35k rows): `Learned::maintain()`, run by
the plugin when the keyboard closes, on its writer thread.
