// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/corrector.h"
#include "parrotfish/languagetracker.h"

#include <memory>
#include <unordered_map>
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
    // Share of the probability the best correction needs.
    double gentleConfidence = 0.5;
    double normalConfidence = 0.5;
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
    size_t strongMinLength = 2;
    // An uncertain tap takes the touch-down key only if that makes the word
    // this many nats more likely than the lift key.
    double tapSwitchMargin = 2.0;
    // Weight of the previous word when ranking corrections: P(word) is
    // pairWeight × P(word | previous word) + (1 - pairWeight) × P(word),
    // from the starting word-pair data (interpolated word-pair model).
    double pairWeight = 0.5;
    // Cost of "the typed letters are an intended word missing from the
    // dictionaries": unknownBase + unknownPerLetter × letters, plus the taps.
    double unknownBase = 6.0;
    double unknownPerLetter = 0.5;
    // Normal: an unknown word is less likely, so fixes apply more often.
    double normalUnknownBase = 8.0;
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
