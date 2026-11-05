// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "parrotfish/dictionary.h"
#include "parrotfish/keymodel.h"

#include <string>
#include <unordered_map>
#include <limits>
#include <vector>

namespace parrotfish {

// One tap: the key typed (where the finger lifted) and, if different, the
// key first touched. `ch` is lowercase; characters typed from a popup or
// the symbol view are taken as typed (`onLayout` false). When the keyboard
// typed the touch-down key instead (the word fitted it better), `ch` is that
// key, `initial` the lift key and `chLifted` false: `ch` is always what the
// word shows, and the lift key keeps its weight.
struct Tap {
    char32_t ch = 0;
    char32_t initial = 0;    // 0: same as ch; else the tap's other key
    bool upper = false;      // typed with Shift or Caps Lock
    bool chLifted = true;    // ch is where the finger lifted (else touched down)
};

// All costs are negative natural logs of probabilities.
struct CorrectorParams {
    float liftWeight = 0.65f;      // lift key vs touch-down key, when they differ
    float farTap = 3.5f;           // a tap costing more than this counts as an edit
    float minTapProb = 2e-4f;      // ignore keys less likely than this
    float accentOmitted = 1.2f;    // typed e, meant é
    float accentNoKey = 0.6f;      // the same when é has no key on the layout:
                                   // the plain letter is how it is typed
    float accentWrong = 3.0f;      // typed é, meant e (or another accent)
    float deletion = 3.5f;         // a letter missing from the taps
    float insertion = 6.0f;        // an extra tap
    float transposition = 2.5f;    // two neighbouring taps swapped
    float doubledDeletion = 2.0f;  // the second of two equal letters missing (aparte)
    float doubledInsertion = 2.5f; // an extra tap of the key just tapped (coomputer)
                                   // (both used only when cheaper than deletion, insertion)
    float punctuationSkipped = 0.8f;   // ' or - of the word not typed
    float ruleCost = 0.4f;         // a spelling rule (ae → ä, e' → è)
    int maxEdits = 2;              // insertions + deletions + transpositions
    float lmWeight = 0.6f;         // scale of word frequency against tap evidence
    // Typed letters that are exactly a listed misspelling (im, perche,
    // fur) are that word written that way, not a slip onto it: its target
    // costs this many nats less. Without it, im became in (the word
    // frequencies come from written text, where I'm is rare).
    // sentence-bench (2026-11-01), 0 -> 2.5: German accent misspellings
    // fixed 64.6 -> 70.8 %, French misspellings +0.6 points, nothing fixed
    // wrong more; 3: more fixed wrong. Next-word keystrokes saved -0.1 to
    // -0.2 points (a prefix that is a misspelling, fur..., ranks für).
    // Not after a word with a capital (Mr Cant: Engine::process).
    float exactMisspellingBonus = 2.5f;
    size_t maxResults = 12;
    bool completions = false;      // also return longer words (prefix match)
    float completionCost = 0.3f;   // per letter added beyond the taps
    float completionSlack = 2.5f;  // completions only for prefixes this close to the taps
};

struct Candidate {
    std::u32string word;   // as stored in the dictionary (case not applied)
    double cost = 0;       // tap cost + word cost (lower is better)
    double tapCost = 0;    // -log P(taps | word)
    double wordCost = 0;   // -log P(word) in its language, incl. language prior
    int edits = 0;
    int changes = 0;       // letters changed, added, removed or swapped (accents don't count)
    int extraLetters = 0;  // > 0 for completions
    uint32_t attrs = 0;    // format::AttrFlag of the dictionary entry
    bool viaTarget = false;   // produced from a misspelling/shortcut entry
    uint8_t freq = 0;      // the word's frequency (0..255) behind wordCost
};

// Finds the words that best explain a sequence of taps in one dictionary
// (a noisy-channel search over the trie with an edit budget).
class Corrector
{
public:
    Corrector(const Dictionary &dict, const KeyModel &keys, CorrectorParams params = {});

    // `logPrior` is added to every word's log probability (language weight).
    // Words costing `bound` or more are not searched for (another language
    // already has far better ones).
    std::vector<Candidate> search(const std::vector<Tap> &taps, double logPrior = 0.0,
                                  double bound = std::numeric_limits<double>::infinity()) const;

    const CorrectorParams &params() const { return m_params; }

    // -log P(taps | word) for one given word of the typed length (missing,
    // extra and swapped letters within the edit budget); infinity if the
    // taps can't be this word. For words suggested by the context.
    double matchCost(const std::vector<Tap> &taps, const std::u32string &word, int *edits = nullptr,
                     int *changes = nullptr) const;

    // The taps against words that are not in the dictionary (learned
    // words), with one setup for all: each word typed in full, or with
    // completions on, its beginning typed without a slip (as search()
    // completes). tapCost infinity: the taps can't be that word.
    struct WordMatch {
        double tapCost = std::numeric_limits<double>::infinity();
        int edits = 0;
        int changes = 0;
        int extraLetters = 0;
    };
    std::vector<WordMatch> matchWords(const std::vector<Tap> &taps, const std::vector<std::u32string> &words) const;
    // The same with the words already as alphabet indices (letters()):
    // for a list matched at every tap.
    std::vector<WordMatch> matchLetters(const std::vector<Tap> &taps, const std::vector<std::vector<int>> &words) const;
    // A word as alphabet indices for matchLetters (a capital not in the
    // alphabet: its lowercase letter); empty if a letter isn't in it.
    std::vector<int> letters(const std::u32string &word) const;

    struct Search;   // (corrector.cpp)

private:
    // The alphabet index of each letter (with lowerFallback, a capital not
    // in the alphabet: its lowercase letter). False if a letter isn't in it.
    bool alphabetIndices(const std::u32string &word, std::vector<int> &out, bool lowerFallback) const;
    const Dictionary &m_dict;
    const KeyModel &m_keys;
    CorrectorParams m_params;
    std::vector<int> m_keyOfLetter;      // per alphabet entry: key index or -1
    std::vector<int> m_keyOfBase;        // key of the base letter, or -1
    std::unordered_map<char32_t, char32_t> m_baseOf;   // lowercase letter → base letter
    std::vector<uint8_t> m_punctuation;  // per alphabet entry: ' - . (may be left out)
    std::vector<uint8_t> m_ruleOutput;   // per alphabet entry: made by a spelling rule

public:
    char32_t baseOf(char32_t ch) const;
};

// Helpers for tests and tools: taps for a plain string (no noise).
std::vector<Tap> tapsFor(const std::u32string &typed);

// Sets one parameter from `name=value` (bench and tools). False if unknown.
bool setParam(CorrectorParams &p, const std::string &assignment);

} // namespace parrotfish
