// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/dictionary.h"

#include <string>
#include <vector>

namespace parrotfish {

// Hidden tuning parameters (plan §5). Defaults come from the mixed-language
// benchmark (tests/bench/lang-bench.cpp).
struct LanguageParams {
    // Probability that the language changes between two words. Together with
    // maxEvidence it sets the "switch cost": the evidence needed to change the
    // leading language.
    double switchProb = 0.01;
    // Cap, in nats, on how much one word can favour one language over
    // another. Below log((1 - s) / s) a single word can't flip the lead.
    double maxEvidence = 3.0;
    // Weight of words of up to two letters (a, in, die), 0..1. Measured: 1
    // is best, since shared short words already carry little evidence.
    double shortWordWeight = 1.0;
    // A word missing from a dictionary counts as this frequency (0..255).
    int unknownFreq = 30;
    // A form the dictionary lacks but its rules make (Dictionary::isWordForm)
    // counts as this frequency: about the rarest words of the AOSP lists.
    int formFreq = 60;
};

// Follows which typing language the text is in: a two-or-more-state hidden
// Markov model over the committed words, updated with the forward algorithm.
// Each language keeps a probability; nothing switches on or off.
class LanguageTracker
{
public:
    LanguageTracker() = default;
    LanguageTracker(std::vector<const Dictionary *> dicts, LanguageParams params = {});

    // Starts a new text. `prior` (one weight per language) defaults to uniform.
    void reset(const std::vector<double> &prior = {});
    // Updates the weights with one committed word.
    void observe(const std::u32string &word);

    const std::vector<double> &weights() const { return m_weights; }
    size_t size() const { return m_weights.size(); }
    int leading() const;          // index of the most likely language
    double confidence() const;    // its probability

    // log P(word) per language, as used for the evidence (for tests).
    std::vector<double> logLikelihoods(const std::u32string &word) const;

    const LanguageParams &params() const { return m_params; }

private:
    std::vector<const Dictionary *> m_dicts;
    LanguageParams m_params;
    std::vector<double> m_weights;
};

bool setParam(LanguageParams &p, const std::string &assignment);

} // namespace parrotfish
