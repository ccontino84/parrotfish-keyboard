// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace parrotfish {

// When words are learned and forgotten.
struct LearnParams {
    // A word missing from the dictionaries is learned once it is kept in
    // this many different texts, within newWordWindow words typed in its
    // language; else it is dropped.
    int newWordTexts = 3;
    double newWordWindow = 5000;
    // Every fadeEvery words typed in a language, its counts are multiplied
    // by fade; words and pairs below dropBelow are forgotten. Scheduled when
    // the words are typed, done by maintain().
    double fadeEvery = 10000;
    double fade = 0.8;
    double dropBelow = 0.5;
    // At most this many words (learned or not yet) and word pairs per
    // language; the least used go first (of words used as often, the one
    // kept longest ago). Trimmed by maintain() once 10% over.
    size_t maxWords = 20000;
    size_t maxPairs = 50000;
};

bool setParam(LearnParams &p, const std::string &assignment);

// What the user typed, per language: how often they use words, the words
// they use that the dictionaries lack, and which word they type after
// which. No Qt, no storage: the plugin keeps it between sessions, the
// benches fill it as the plugin would. The engine reads it (Engine::
// setLearned). Words are keyed by their lowercase spelling.
class Learned
{
public:
    struct Word {
        std::u32string spelling;   // as typed (a new word's capitals; else as last typed)
        double count = 0;          // times kept, faded
        bool isNew = false;        // not in a typing language's dictionary when first kept
        bool pending = false;      // a new word not learned yet: not used
        int texts = 0;             // texts it was kept in (until learned)
        double firstSeen = 0;      // Language::typed when first kept (until learned)
        uint64_t lastText = 0;     // the text it was last kept in (this session)
        double lastUsed = 0;       // Language::typed when last kept
    };
    struct Language {
        double typed = 0;        // words kept in this language: the clock for fading and the window
        double nextFade = 0;     // typed at the next fading (due from then on: maintain())
        double total = 0;        // counts of the words in use (not pending)
        std::unordered_map<std::u32string, Word> words;
        // previous word ("": a sentence start) → next word → count, and the
        // previous word's total.
        std::unordered_map<std::u32string, std::unordered_map<std::u32string, double>> next;
        std::unordered_map<std::u32string, double> nextTotal;
        size_t pairs = 0;

        // A word in use (learned, not pending), or null.
        const Word *word(const std::u32string &key) const;
    };

    explicit Learned(LearnParams params = {});
    const LearnParams &params() const { return m_params; }

    // A text ends (the keyboard closes, the field is emptied): a new word
    // counts once per text.
    void endText();
    // The user kept `word` in `language`: typed and left, picked, or an
    // autocorrection kept (not one undone into something else). `known`:
    // a word of a typing language's dictionary (Engine::isKnown). `prev`:
    // the word before it in the same sentence (empty at a sentence start),
    // `prevKnown` whether it is in a typing language's dictionary: pairs are
    // learned only between real words (known or learned).
    void add(const std::string &language, const std::u32string &word, bool known,
             const std::u32string &prev, bool prevKnown);
    // Fading and trimming, scheduled by add(): each changes every count of a
    // language, which is then stored whole (Changes::rewrite). The plugin
    // calls maintain() when the keyboard closes, not while the user types.
    bool maintenanceDue() const;
    void maintain();

    const Language *language(const std::string &code) const;
    std::vector<std::string> languages() const;
    // Whether `word` is a learned new word in any language.
    bool isLearnedNew(const std::u32string &word) const;
    // Changes whenever the data does (the engine's caches follow it).
    uint64_t version() const { return m_version; }
    // Changes when the learned new words do (learned, forgotten, dropped).
    uint64_t newWordsVersion() const { return m_newWordsVersion; }

    // Forgets a word in every language: its count, its pairs.
    void forget(const std::u32string &word);
    void clear();

    // Storage (the plugin): what changed since the last takeChanges(). A
    // word or pair listed is stored as it is now, or deleted if it is gone.
    struct Changes {
        bool cleared = false;                // everything was forgotten first
        std::set<std::string> rewrite;       // languages to store whole (faded, trimmed)
        std::set<std::string> languages;     // their typed and nextFade
        std::set<std::pair<std::string, std::u32string>> words;   // (language, key)
        std::set<std::tuple<std::string, std::u32string, std::u32string>> pairs;   // (language, prev, word)
        bool empty() const
        {
            return !cleared && rewrite.empty() && languages.empty() && words.empty() && pairs.empty();
        }
    };
    Changes takeChanges();
    // Loading what was stored (no changes recorded), then finishRestore().
    void restoreLanguage(const std::string &language, double typed, double nextFade);
    void restoreWord(const std::string &language, const std::u32string &key, const Word &word);
    void restorePair(const std::string &language, const std::u32string &prev, const std::u32string &word,
                     double count);
    void finishRestore();

private:
    Language &lang(const std::string &code);
    bool overCaps(const Language &l) const;
    void fadeAll(Language &l);
    void trim(Language &l);
    void dropPair(Language &l, const std::u32string &prev, const std::u32string &key);

    LearnParams m_params;
    std::unordered_map<std::string, Language> m_languages;
    uint64_t m_text = 1;
    uint64_t m_version = 1;
    uint64_t m_newWordsVersion = 1;
    Changes m_changes;
};

// Whether a word may be learned as new: two letters at least, no digit,
// not in capitals only beyond four letters (shouting, codes).
bool learnableNewWord(const std::u32string &word);

} // namespace parrotfish
