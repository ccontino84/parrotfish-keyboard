// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/corrector.h"
#include "parrotfish/languagetracker.h"

#include <memory>
#include <unordered_map>
#include <limits>
#include <string>
#include <vector>

namespace parrotfish {

// Strong: Normal, and valid words may be fixed from the previous word
// (plan §6; until that exists, the same as Normal).
enum class Strength { Off, Gentle, Normal, Strong };

struct EngineParams {
    CorrectorParams corrector;
    LanguageParams language;
    // A language at least this likely makes its words count as valid (§5:
    // the valid-word threshold).
    double validThreshold = 0.05;
    // Autocorrect acts only for a word of a language at least this likely
    // (§5: autocorrect confidence). Below it, autocorrect is paused.
    double autocorrectConfidence = 0.7;
    // The first word of a text, whose language is only the starting lean
    // (the language of the last text: 0.6 of three): a correction that is a
    // word of a language at least this likely may still apply, if it is the
    // best correction with all languages equally likely too and its share of
    // the probability (the lower of the two) is at least firstWordShare.
    double firstWordConfidence = 0.5;
    double firstWordShare = 0.7;
    // Share of the probability the best correction needs. The strengths
    // share one model and differ only here: Gentle asks more. Up to
    // alpha85 Gentle had its own unknownBase (6, against 8) and 0.5;
    // with one base, 0.875 (sentence-bench, Gentle, 5 languages x 2
    // seeds, 2026-11-01): typos fixed 32.5 -> 32.3 %, fixed wrong 0.41 ->
    // 0.26, unknown words changed 0.13 -> 0.16; real misspellings fixed
    // the same or more (accents it +8.7, fr +3.6 points), fixed wrong
    // lower (it 2.6 -> 0, de 0.9 -> 0.3). 0.85: more fixed, more names
    // changed (0.19); 0.9: names as before, 2.4 points fewer fixed.
    double gentleConfidence = 0.875;
    double normalConfidence = 0.5;
    // Normal and Strong ask more of shorter words, which have more
    // neighbours and are more often names (sentence-bench -L sweep,
    // 2026-10-31: at 0.5, 3-4 letters had 2.4-3.6 % wrong fixes and 12-13 %
    // of names changed; at 0.8, 0.8-1.5 % and 4-9 %).
    double normalTwoLetterConfidence = 0.7;   // 2 letters
    double normalShortConfidence = 0.8;       // 3 and 4 letters
    // Normal (and Strong) leave shorter input alone: a single letter is not
    // replaced (t → it, p → to were; the benches don't type such words).
    size_t normalMinLength = 2;
    // Gentle leaves shorter input alone (plan §6: short words are Normal only).
    size_t gentleMinLength = 3;
    // Gentle changes at most this many letters: one in words of up to
    // gentleShortLength letters, two in longer ones.
    size_t gentleShortLength = 5;
    // Gentle fixes two typed letters when one changes to a neighbouring key
    // and the correction has at least this share of the probability.
    double gentleTwoLetterConfidence = 0.7;
    // ... and also when only an accent differs (ca → ça).
    bool gentleTwoLetterAccents = true;
    // Strong: after a previous word (not at a sentence start), a valid word
    // of at least strongMinLength letters becomes another valid word one slip
    // away (a neighbouring key or two letters swapped) if that word is at
    // least strongMargin nats likelier, previous word and taps included
    // (5: about 150 times; sentence-bench -R, plan §6).
    double strongMargin = 5.0;
    // A word of a typing language that seems unlikely in the text (weight
    // below validThreshold: Italian "ti" in English text) is replaced only
    // if the fix is this many nats likelier than the word read as a word of
    // its language, with that language's weight (so it follows the text: an
    // Italian word weighs more once Italian is being typed). Infinity: never
    // replaced (the hard block up to alpha68). 12 (sentence-bench, en/it/de,
    // 2026-10-31): 2-3 more typos fixed in 100 (thier is the German surname
    // Thier, tp the abbreviation TP), and no word of another language changed in
    // mixed texts (tools/eval/mix-sentences.py; at 8: 0.4 % of single foreign
    // words, un → in).
    double otherLanguageMargin = 12.0;
    // Two words typed as one (plan §6 fix 6): a string that is no word of a
    // typing language may be two words of one language, each typed as it
    // is, with the space left out (thisis: missingSpaceCost nats) or typed
    // as a key above the spacebar (thisbis: spaceKeyCost), scored like any
    // other candidate. Each word needs splitMinLength letters. Infinity:
    // never split.
    double missingSpaceCost = 3.5;
    double spaceKeyCost = 4.0;
    size_t splitMinLength = 2;
    size_t strongMinLength = 2;
    // An uncertain tap takes the touch-down key only if that makes the word
    // this many nats more likely than the lift key.
    double tapSwitchMargin = 2.0;
    // Weight of the previous word when ranking corrections: P(word) is
    // pairWeight × P(word | previous word) + (1 - pairWeight) × P(word),
    // from the starting word-pair data (interpolated word-pair model).
    double pairWeight = 0.5;
    // Cost of "the typed letters are an intended word missing from the
    // dictionaries": unknownBase plus the taps, plus unknownLetterWeight ×
    // (-log P(letters) − unknownLetterRef) by the language's letter model
    // (Dictionary::letterCost; 16 nats: a word of the list of seven or eight
    // letters), mixed over the languages by their weights. Letters that look
    // like a word of the text's languages (a name: Kaito, Müller) make it
    // likelier than unlikely ones (xqzt), so fewer names are changed.
    // Without a letter model, with unknownLetterWeight 0, for a word's
    // letters typed without its accents, or for a word of another language:
    // unknownPerLetter × letters.
    // Letters that look like a word and are one change from one (the
    // likeliest fix) are mostly that word misspelled when they are long
    // (recieve, goverment, propio), often a name when short (Ugo, Norn): what
    // the letter model takes off unknownPerLetter × letters then shrinks by
    // unknownLongLetterCost for each letter past unknownShortLength.
    // sentence-bench -L, en/it/de/fr/es, 12 runs each, Normal (2026-11-01),
    // per letter only -> letter model -> with the shrink: 44.4 -> 47.5 ->
    // 51.9 % of typos fixed, 1.12 -> 0.98 -> 1.07 % fixed wrong, 1.23 ->
    // 0.67 -> 0.91 % of unknown words changed (most of the rest are words
    // without their accents); 3-4 letters: 47.2 -> 45.5 -> 45.5 % fixed,
    // 3.8 -> 1.8 -> 1.8 % changed. Gentle 29.8 -> 34.1 -> 36.4, 0.63 ->
    // 0.48 -> 0.51, 0.40 -> 0.23 -> 0.25. Real misspellings (-W, lists of
    // common misspellings), fixed: en 41 -> 23 -> 40 %, it 57 -> 32 -> 55,
    // de 51 -> 30 -> 51, fr 57 -> 25 -> 56, es 39 -> 23 -> 37; fixed wrong
    // lower than per letter only. The gain is in typos with unlikely letters
    // (h3llo, thisbis) and long typos; short typos that look like words are
    // fixed less (they look like short names). Weights 0.35-0.45 do as
    // well, 0.2 worse; set the level with unknownLetterRef:
    // unknownBase also moves the accent fixes (perche). The shrink
    // from 3 letters on (1 nat) fixes short typos as per letter only, at
    // 1.95 % of 3-4-letter names changed; a cap on what the letter model
    // takes off, at any length, does worse.
    double unknownBase = 8.0;
    double unknownPerLetter = 0.5;
    double unknownLetterWeight = 0.4;
    double unknownLetterRef = 16.0;
    size_t unknownShortLength = 4;
    double unknownLongLetterCost = 1.5;
    // ... and when it is two changes away, from unknownTwoChangeLength
    // letters (conoleanno: compleanno, letters that look Italian).
    // sentence-bench (2026-11-01), off -> 9: real misspellings fixed +0.2
    // to +0.6 points (en, fr, de, es), fixed wrong and unknown words
    // changed the same; 8 and less: more fixed wrong (de 1.4 -> 1.6 %).
    size_t unknownTwoChangeLength = 9;
    // A word typed with a capital first letter (not all capitals) after a
    // word, not at a sentence start, is rarely a word the dictionaries store
    // in lower case (cant: can't) and often a name (I met Mr Cant), or a
    // word they store with its capital (London, German nouns): the words
    // stored in lower case cost capitalLowerCost nats more; in the
    // languages that capitalise their nouns (German), where nouns made of
    // verbs and adjectives (beim Essen, das Gute) and the polite Sie are
    // capitalised too, capitalLowerCostNouns. Words stored only in lower
    // case are typed with a capital after a word in 0.3-0.8 % of Tatoeba's
    // (5-6 nats), 3-6 % of news and informal text (3 nats: titles, brands,
    // missed sentence starts), German 4-5 %. sentence-bench -L, Normal
    // (2026-11-01), 0 -> 4 (German 2): unknown words changed en 0.47 ->
    // 0.18 %, it 1.37 -> 0.62, fr 1.38 -> 1.02, es 0.83 -> 0.53, de 0.70 ->
    // 0.52; typos fixed 0 to -0.3 points, fixed wrong 0 to -0.2; next-word
    // keystrokes saved +0.1 to +1.2 (a capital favours names and nouns).
    // Real misspellings (-W, in news text) fixed -0.5 to -2.4 points, all
    // of them capitalised words in titles (Cheif Editior). Costs 3 to 5 do
    // about as well; 4 keeps Mr Cant (2 does not). German at 4: typos fixed
    // -0.7 points.
    double capitalLowerCost = 4.0;
    double capitalLowerCostNouns = 2.0;
    // Tap distances up and down: 0 = in key widths, 1 = in key heights
    // (KeyModel heightShare). Also sets which keys are neighbours.
    double keyHeightShare = 0.0;
    // Languages are searched from the most likely down; a later one only
    // for words costing less than the best word so far plus this margin
    // (nats; e^-margin is how much less likely such a word is). 0: every
    // language is searched in full. The margin must stay well above the
    // starting leans of the first word (×3 × ×2: 1.8 nats), which the
    // first-word check takes out again (probEqual).
    double searchMargin = 6.0;
    // Languages less likely than this are not searched at all (their
    // words still count as valid, and the tracker still follows them).
    double searchMinWeight = 1e-6;
    Strength strength = Strength::Gentle;
};

bool setParam(EngineParams &p, const std::string &assignment);

struct Suggestion {
    std::u32string word;      // with the typed capitalisation applied
    double cost = 0;          // combined over languages (lower is better)
    uint32_t languages = 0;   // bit i: a word of language i
    uint32_t attrs = 0;       // format::AttrFlag (of the best language)
    int edits = 0;
    int changes = 0;          // letters changed, added, removed or swapped
    bool completion = false;  // longer than the taps (only with completions on)
};

struct Result {
    std::u32string typed;           // the taps as typed
    bool typedIsWord = false;       // valid in a likely language
    std::vector<Suggestion> suggestions;   // best first; never contains `typed`
    int autocorrect = -1;           // index into suggestions applied on Space, or -1
    double confidence = 0;          // share of probability of suggestions[0]
    // Strong only: the autocorrection replaces a valid word, this many nats
    // likelier than it.
    bool realWordFix = false;
    double margin = 0;
};

// The engine for one text field: all typing languages at once, weighted
// by the language tracker.
class Engine
{
public:
    explicit Engine(EngineParams params = {});
    ~Engine();

    bool addLanguage(const std::string &dictPath, std::string *error = nullptr);
    void clearLanguages();
    size_t languageCount() const { return m_dicts.size(); }
    std::string language(size_t i) const { return m_dicts[i]->language(); }
    int languageIndex(const std::string &code) const;   // -1 if not loaded
    const Dictionary &dictionary(size_t i) const { return *m_dicts[i]; }

    void setLayout(const KeyModel &keys);
    const EngineParams &params() const { return m_params; }
    void setStrength(Strength strength) { m_params.strength = strength; }
    // Replaces all parameters (the language weights start over).
    void setParams(const EngineParams &params);

    // Starts a text: optional starting weights, then the words already in
    // the field before the cursor.
    void startText(const std::u32string &textBefore = {}, const std::vector<double> &prior = {});
    // A word was committed (typed, chosen or autocorrected).
    void commit(const std::u32string &word);

    Result process(const std::vector<Tap> &taps) const;

    // Next-word suggestions for the text before the cursor, when no word is
    // being typed: sentence starters at the start of a sentence (capitalised),
    // otherwise the usual next words of the last word. Empty when the text
    // ends inside a word. Uses the current language weights.
    std::vector<Suggestion> predict(const std::u32string &textBefore, size_t max = 10) const;

    // An uncertain tap: the finger went down on `initial` and lifted on
    // `lifted`. Returns the letter that makes `word` (typed so far) the more
    // likely beginning of a word in the likely languages; `lifted` unless
    // `initial` is clearly better.
    char32_t resolveTap(const std::u32string &word, char32_t lifted, char32_t initial) const;

    const LanguageTracker &tracker() const { return m_tracker; }

private:
    void rebuild();
    // `word` is `typed` with one letter changed to a neighbouring key.
    bool neighbourSwap(const std::u32string &typed, const std::u32string &word) const;
    // Two neighbouring letters swapped (from → form), accents aside.
    bool swapped(const std::u32string &typed, const std::u32string &word) const;

    // Where the word being typed stands in the text.
    struct Context {
        bool sentenceStart = true;
        bool insideWord = false;     // the text ends with letters (no space)
        std::u32string prev;         // the previous word, if any
    };
    static Context parseContext(const std::u32string &textBefore);
    Context m_context;
    size_t m_textWords = 0;   // words in the text so far (startText, commit)

    // Lowercase without accents, via the dictionaries' letter tables.
    std::u32string fold(const std::u32string &word) const;
    std::unordered_map<char32_t, char32_t> m_base;

    EngineParams m_params;
    std::vector<std::unique_ptr<Dictionary>> m_dicts;
    std::vector<Key> m_layout;   // as passed to setLayout
    float m_layoutSigma = 0.5f;
    KeyModel m_keys;             // m_layout with keyHeightShare
    std::vector<std::unique_ptr<Corrector>> m_correctors;
    LanguageTracker m_tracker;
};

// Splits text into words: letters with inner apostrophes and hyphens
// (l'anno, don't, E-Mail). Typographic apostrophes become '.
std::vector<std::u32string> words(const std::u32string &text);

// Applies the capitalisation of the taps to a word: a capital first tap
// capitalises the first letter, all-capital taps (two or more) the whole word.
std::u32string applyCase(const std::u32string &word, const std::vector<Tap> &taps);

} // namespace parrotfish
