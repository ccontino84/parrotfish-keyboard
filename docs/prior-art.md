<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LicenseRef-TBD
-->

# Prior art

For each engine feature, the published method it implements.

**Status:** the references below are written from memory and are
**not verified yet**. Check each one (title, venue, year) before the
project is published.

| Feature | Code | Method | Reference (to verify) |
|---|---|---|---|
| Correction candidates: one letter missing, extra, changed, or two swapped | `engine/src/corrector.cpp` | edit operations incl. transposition | F. J. Damerau, "A technique for computer detection and correction of spelling errors", Comm. ACM 7(3), 1964; V. I. Levenshtein, 1965/1966 |
| Searching the dictionary trie with an error budget | `Corrector::Search::visit` | error-tolerant search over a trie / finite-state lexicon | K. Oflazer, "Error-tolerant finite-state recognition with applications to morphological analysis and spelling correction", Computational Linguistics 22(1), 1996 |
| Ranking: P(word) × P(typed letters given word) | `corrector.cpp`, `engine.cpp` | noisy-channel spelling correction | M. D. Kernighan, K. W. Church, W. A. Gale, "A spelling correction program based on a noisy channel model", COLING 1990 |
| Tap probabilities from key distances | `engine/src/keymodel.cpp` | spatial model of touch-screen key presses combined with a language model | J. Goodman, G. Venolia, K. Steury, C. Parker, "Language modeling for soft keyboards", AAAI 2002 |
| Weight of the word frequencies against the typing evidence | `CorrectorParams::lmWeight` | language-model scale factor of speech recognisers | e.g. L. R. Bahl, F. Jelinek, R. L. Mercer, "A maximum likelihood approach to continuous speech recognition", IEEE PAMI 5(2), 1983 |
| Following the language word by word | `engine/src/languagetracker.cpp` | hidden Markov model, forward algorithm | L. R. Rabiner, "A tutorial on hidden Markov models and selected applications in speech recognition", Proc. IEEE 77(2), 1989 |
| Language evidence from word frequencies | `LanguageTracker::observe` | statistical language identification | T. Dunning, "Statistical identification of language", NMSU tech. report MCCS-94-273, 1994; W. B. Cavnar, J. M. Trenkle, "N-gram-based text categorization", 1994 |
| Accent-free matching | `corrector.cpp` (`accentOmitted`) | Unicode canonical decomposition, base letter comparison | The Unicode Standard, normalization forms (UAX #15) |
| Searching the likeliest language first and cutting the others short | `Engine::process` (`searchMargin`), `Corrector::search` (`bound`) | branch and bound: a search drops branches that can't beat the best solution found so far | A. H. Land, A. G. Doig, "An automatic method of solving discrete programming problems", Econometrica 28(3), 1960 |

Not implemented in milestone 1 (listed for later): next-word prediction from
word pairs (n-gram language models, e.g. Jelinek 1980s; Katz back-off 1987).
