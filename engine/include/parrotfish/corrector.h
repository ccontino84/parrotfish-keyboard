// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/dictionary.h"
#include "parrotfish/keymodel.h"

#include <string>
#include <vector>

namespace parrotfish {

// One tap: the key typed (where the finger lifted) and, if different, the
// key first touched. `ch` is lowercase; characters typed from a popup or
// the symbol view are taken as typed (`onLayout` false).
struct Tap {
    char32_t ch = 0;
    char32_t initial = 0;    // 0: same as ch
    bool upper = false;      // typed with Shift or Caps Lock
};

// All costs are negative natural logs of probabilities.
struct CorrectorParams {
    float liftWeight = 0.65f;      // lift key vs touch-down key, when they differ
    float farTap = 3.5f;           // a tap costing more than this counts as an edit
    float minTapProb = 2e-4f;      // ignore keys less likely than this
    float accentOmitted = 1.2f;    // typed e, meant é
    float accentWrong = 3.0f;      // typed é, meant e (or another accent)
    float deletion = 4.5f;         // a letter missing from the taps
    float insertion = 4.5f;        // an extra tap
    float transposition = 4.0f;    // two neighbouring taps swapped
    float punctuationSkipped = 0.8f;   // ' or - of the word not typed
    float ruleCost = 0.4f;         // a spelling rule (ae → ä, e' → è)
    int maxEdits = 2;              // insertions + deletions + transpositions
    float lmWeight = 0.6f;         // scale of word frequency against tap evidence
    size_t maxResults = 12;
    bool completions = false;      // also return longer words (prefix match)
    float completionCost = 0.3f;   // per letter added beyond the taps
};

struct Candidate {
    std::u32string word;   // as stored in the dictionary (case not applied)
    double cost = 0;       // tap cost + word cost (lower is better)
    double tapCost = 0;    // -log P(taps | word)
    double wordCost = 0;   // -log P(word) in its language, incl. language prior
    int edits = 0;
    int extraLetters = 0;  // > 0 for completions
    uint32_t attrs = 0;    // format::AttrFlag of the dictionary entry
    bool viaTarget = false;   // produced from a misspelling/shortcut entry
};

// Finds the words that best explain a sequence of taps in one dictionary
// (a noisy-channel search over the trie with an edit budget).
class Corrector
{
public:
    Corrector(const Dictionary &dict, const KeyModel &keys, CorrectorParams params = {});

    // `logPrior` is added to every word's log probability (language weight).
    std::vector<Candidate> search(const std::vector<Tap> &taps, double logPrior = 0.0) const;

    const CorrectorParams &params() const { return m_params; }

private:
    struct Search;
    const Dictionary &m_dict;
    const KeyModel &m_keys;
    CorrectorParams m_params;
    std::vector<int> m_keyOfLetter;      // per alphabet entry: key index or -1
    std::vector<int> m_keyOfBase;        // key of the base letter, or -1
};

// Helpers for tests and tools: taps for a plain string (no noise).
std::vector<Tap> tapsFor(const std::u32string &typed);

// Sets one parameter from `name=value` (bench and tools). False if unknown.
bool setParam(CorrectorParams &p, const std::string &assignment);

} // namespace parrotfish
