<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Prior art

For each engine feature, the published method it implements. All
references were checked against library records (title, authors, venue,
year, pages) on 2026-11-05.

| Feature | Code | Method | Reference |
|---|---|---|---|
| Correction candidates: one letter missing, extra, changed, or two swapped | `engine/src/corrector.cpp` | edit operations incl. transposition | F. J. Damerau, "A technique for computer detection and correction of spelling errors", Communications of the ACM 7(3), 171–176, 1964, doi:10.1145/363958.363994; V. I. Levenshtein, "Binary codes capable of correcting deletions, insertions, and reversals", Soviet Physics Doklady 10(8), 707–710, 1966 (Russian original 1965) |
| Searching the dictionary trie with an error budget | `Corrector::search` (`Corrector::Search`) | error-tolerant search over a trie / finite-state lexicon | K. Oflazer, "Error-tolerant finite-state recognition with applications to morphological analysis and spelling correction", Computational Linguistics 22(1), 73–89, 1996 |
| Ranking: P(word) × P(typed letters given word) | `corrector.cpp`, `engine.cpp` | noisy-channel spelling correction | M. D. Kernighan, K. W. Church, W. A. Gale, "A spelling correction program based on a noisy channel model", COLING 1990, vol. 2, 205–210 |
| Tap probabilities from key distances | `engine/src/keymodel.cpp` | spatial model of touch-screen key presses combined with a language model | J. Goodman, G. Venolia, K. Steury, C. Parker, "Language modeling for soft keyboards", AAAI 2002, 419–424 |
| Weight of the word frequencies against the typing evidence | `CorrectorParams::lmWeight` | language-model scale factor of speech recognisers | e.g. L. R. Bahl, F. Jelinek, R. L. Mercer, "A maximum likelihood approach to continuous speech recognition", IEEE Trans. PAMI 5(2), 179–190, 1983 |
| The previous word in the ranking and the next-word suggestions | `EngineParams::pairWeight`, `engine/src/search.cpp` | word-pair (bigram) model interpolated with word frequencies | F. Jelinek, R. L. Mercer, "Interpolated estimation of Markov source parameters from sparse data", in E. S. Gelsema, L. N. Kanal (eds.), Pattern Recognition in Practice, North-Holland, 381–397, 1980 |
| Two words typed as one (a missing or mistyped space) | `engine/src/search.cpp` (`trySplit`) | correction of run-on words | K. Kukich, "Techniques for automatically correcting words in text", ACM Computing Surveys 24(4), 377–439, 1992, doi:10.1145/146370.146380 |
| Strong: a real word fixed into a far likelier one after the previous word | `EngineParams::strongMargin`, `engine/src/decide.cpp` | real-word error correction with a word n-gram model and a noisy channel | E. Mays, F. J. Damerau, R. L. Mercer, "Context based spelling correction", Information Processing & Management 27(5), 517–522, 1991 |
| Following the language word by word | `engine/src/languagetracker.cpp` | hidden Markov model, forward algorithm | L. R. Rabiner, "A tutorial on hidden Markov models and selected applications in speech recognition", Proceedings of the IEEE 77(2), 257–286, 1989, doi:10.1109/5.18626 |
| Language evidence from word frequencies | `LanguageTracker::observe` | statistical language identification | T. Dunning, "Statistical identification of language", Computing Research Laboratory, New Mexico State University, tech. report MCCS-94-273, 1994; W. B. Cavnar, J. M. Trenkle, "N-gram-based text categorization", SDAIR-94, 161–175, 1994 |
| Accent-free matching | `corrector.cpp` (`accentOmitted`) | Unicode canonical decomposition, base letter comparison | The Unicode Standard, normalization forms (UAX #15) |
| Searching the likeliest language first and cutting the others short | `Engine::process` (`searchMargin`), `Corrector::search` (`bound`) | branch and bound: a search drops branches that can't beat the best solution found so far | A. H. Land, A. G. Doig, "An automatic method of solving discrete programming problems", Econometrica 28(3), 497–520, 1960 |
| Learning: the user's own words and next words mixed with the dictionary's, more as more is typed | `Engine::learnedUni`, `EngineParams::learnedWeight` | adaptive language model: a user model interpolated with the static one | R. Kuhn, R. De Mori, "A cache-based natural language model for speech recognition", IEEE Trans. PAMI 12(6), 570–583, 1990, doi:10.1109/34.56193 |
| Learning: counts fade, so words no longer used are forgotten | `Learned::fadeAll` (`LearnParams::fade`) | exponentially decaying cache | P. R. Clarkson, A. J. Robinson, "Language model adaptation using mixtures and an exponentially decaying cache", ICASSP 1997, vol. 2, 799–802 |
