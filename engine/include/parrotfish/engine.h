// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/corrector.h"
#include "parrotfish/languagetracker.h"

#include <memory>
#include <string>
#include <vector>

namespace parrotfish {

enum class Strength { Off, Gentle, Normal };

struct EngineParams {
    CorrectorParams corrector;
    LanguageParams language;
    // A language at least this likely makes its words count as valid (§5:
    // the valid-word threshold).
    double validThreshold = 0.05;
    // Autocorrect acts only for a word of a language at least this likely
    // (§5: autocorrect confidence). Below it, autocorrect is paused.
    double autocorrectConfidence = 0.7;
    // Share of the probability the best correction needs.
    double gentleConfidence = 0.7;
    double normalConfidence = 0.5;
    // Gentle leaves shorter input alone (plan §6: short words are Normal only).
    size_t gentleMinLength = 3;
    // Cost of "the typed letters are an intended word missing from the
    // dictionaries": unknownBase + unknownPerLetter × letters, plus the taps.
    double unknownBase = 6.0;
    double unknownPerLetter = 0.5;
    Strength strength = Strength::Gentle;
};

bool setParam(EngineParams &p, const std::string &assignment);

struct Suggestion {
    std::u32string word;      // with the typed capitalisation applied
    double cost = 0;          // combined over languages (lower is better)
    uint32_t languages = 0;   // bit i: a word of language i
    uint32_t attrs = 0;       // format::AttrFlag (of the best language)
    int edits = 0;
};

struct Result {
    std::u32string typed;           // the taps as typed
    bool typedIsWord = false;       // valid in a likely language
    std::vector<Suggestion> suggestions;   // best first; never contains `typed`
    int autocorrect = -1;           // index into suggestions applied on Space, or -1
    double confidence = 0;          // share of probability of suggestions[0]
};

// The engine for one text field: all typing languages at once, weighted
// by the language tracker.
class Engine
{
public:
    explicit Engine(EngineParams params = {});
    ~Engine();

    bool addLanguage(const std::string &dictPath, std::string *error = nullptr);
    size_t languageCount() const { return m_dicts.size(); }
    std::string language(size_t i) const { return m_dicts[i]->language(); }
    const Dictionary &dictionary(size_t i) const { return *m_dicts[i]; }

    void setLayout(const KeyModel &keys);
    const EngineParams &params() const { return m_params; }

    // Starts a text: optional starting weights, then the words already in
    // the field before the cursor.
    void startText(const std::u32string &textBefore = {}, const std::vector<double> &prior = {});
    // A word was committed (typed, chosen or autocorrected).
    void commit(const std::u32string &word);

    Result process(const std::vector<Tap> &taps) const;

    const LanguageTracker &tracker() const { return m_tracker; }

private:
    void rebuild();

    EngineParams m_params;
    std::vector<std::unique_ptr<Dictionary>> m_dicts;
    KeyModel m_keys;
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
