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

// The measurements behind the values: engine/TUNING.md.
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
    // Gentle: the share of the probability the best correction needs, and
    // its limits below (one edit, few letters: only obvious slips).
    double gentleConfidence = 0.95;
    // Normal and Strong, for letters that are not a word: fix when
    //   P(fix meant) > P(typed meant) * changeCost + P(another word) * (wrongCost - 1),
    // the losses of changing a correct word and of typing a wrong word
    // instead of the slip, against 1 for a slip left as typed, with the
    // probabilities of the calibrated model (unknownBase, unknownLetterWeight,
    // unknownOddsScale).
    double changeCost = 7.0, wrongCost = 5.0;
    // changeCost for 2 and for 3-4 letters (0: changeCost). Short words
    // have more neighbours and are more often names; the typos benches
    // have no unknown words of 2 letters to calibrate them on. A fix that
    // only adds accents or apostrophes uses changeCost at any length.
    double changeCostTwo = 6.0, changeCostShort = 20.0;
    // wrongCost for another word that is the fix but for its ending
    // (received for receive): nearer the meant word than the slip.
    double nearWrongCost = 1.5;
    // The log-odds of "a word missing from the dictionaries" against the
    // words, scaled (they swing too far: fit on the typos benches).
    double unknownOddsScale = 0.6;
    // Letters that are a compound of dictionary words (Dictionary::
    // isCompound: tweedehandsauto, kaputtgemacht) are a missing word this
    // much likelier (nats), against two words typed without the space
    // (heefteen: heeft een), which the word pairs make likely.
    double compoundBonus = 8.0;
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
    // least strongMargin nats likelier, previous word and taps included.
    double strongMargin = 5.0;
    // A word of a typing language that seems unlikely in the text (weight
    // below validThreshold: Italian "ti" in English text) is replaced only
    // if the fix is this many nats likelier than the word read as a word of
    // its language, with that language's weight (so it follows the text: an
    // Italian word weighs more once Italian is being typed). Infinity: never
    // replaced.
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
    double unknownBase = 11.5;
    double unknownPerLetter = 0.5;
    double unknownLetterWeight = 0.5;
    double unknownLetterRef = 16.0;
    size_t unknownShortLength = 4;
    double unknownLongLetterCost = 1.5;
    // ... and when it is two changes away, from unknownTwoChangeLength
    // letters (conoleanno: compleanno, letters that look Italian).
    size_t unknownTwoChangeLength = 9;
    // A word typed with a capital first letter (not all capitals) after a
    // word, not at a sentence start, is rarely a word the dictionaries store
    // in lower case (cant: can't) and often a name (I met Mr Cant), or a
    // word they store with its capital (London, German nouns): the words
    // stored in lower case cost capitalLowerCost nats more; in the
    // languages that capitalise their nouns (German), where nouns made of
    // verbs and adjectives (beim Essen, das Gute) and the polite Sie are
    // capitalised too, capitalLowerCostNouns.
    double capitalLowerCost = 5.0;
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

// Why autocorrect did or did not replace the word (Result::reason): which
// decision of Engine::process ended it. For tools (the benches' candidate
// dump, pf failures); it changes nothing.
enum class Reason : uint8_t {
    None,              // nothing to decide: no taps, no dictionary, or too long
    Applied,           // replaced by the best fix (Strong's real-word fixes too: realWordFix)
    CaseFix,           // replaced: a valid word in its stored capitalisation (haus -> Haus)
    NoCandidate,       // no complete word to fix it to
    Digits,            // a digit in the word: no fix is a slip onto the digit
    LanguageNotSure,   // the fix's language below autocorrectConfidence (first word: firstWord*)
    Offensive,         // the fix is an offensive word
    StrengthOff,       // autocorrect off
    OtherLanguage,     // a word of a less likely typing language: fix not otherLanguageMargin likelier
    TypedIsWord,       // a valid word, not a case fix, Strong's real-word fix not tried
    StrongMargin,      // Strong: the fix is less than strongMargin likelier than the valid word
    SlipShortcut,      // Strong: a slip word (foe) whose fix the previous word doesn't support
    BelowNormal,       // Normal/Strong: the fix's expected loss above leaving the letters
    TooShort,          // fewer letters than normalMinLength / gentleMinLength
    BelowGentle,       // Gentle: confidence below gentleConfidence
    GentleChanges,     // Gentle: more edits or changes than it makes
};
const char *reasonName(Reason reason);

struct Result {
    std::u32string typed;           // the taps as typed
    bool typedIsWord = false;       // valid in a likely language
    std::vector<Suggestion> suggestions;   // best first; never contains `typed`
    int autocorrect = -1;           // index into suggestions applied on Space, or -1
    double confidence = 0;          // share of probability of suggestions[0]
    // Share of "an intended word missing from the dictionaries" in the same
    // total (0 for a valid word). For tools.
    double unknownShare = 0;
    // Strong only: the autocorrection replaces a valid word, this many nats
    // likelier than it.
    bool realWordFix = false;
    double margin = 0;
    Reason reason = Reason::None;
};

// What Engine::search found for some taps, before the decision
// (Engine::decide): the candidates merged over the languages, whether the
// taps are a word, and the context the search saw.
struct Searched {
    std::vector<Tap> taps;
    std::u32string typed;            // the taps as typed
    bool searched = false;           // false: nothing to decide (no taps or dictionaries, too long)
    bool typedIsWord = false;        // valid in a likely language
    bool wordInSomeLanguage = false; // valid in some typing language
    bool validAsTyped = false;       // not only in another case (jaime: Jaime only)
    bool compound = false;           // a compound of a likely language's words (Dictionary::isCompound)
    bool typedHasShortcut = false, typedHasSlipShortcut = false;
    std::u32string caseFix;          // the stored capitalisation, if it differs (haus → Haus)
    std::vector<double> weights;     // the languages' weights
    bool sentenceStart = true;
    std::u32string prev;             // the previous word, if any
    size_t textWords = 0;            // words in the text before
    // One candidate, its spellings in all languages merged (probEqual: with
    // all languages equally likely).
    struct Merged {
        Suggestion s;
        std::u32string key;
        double prob = 0;
        double probEqual = 0;
        double bestCost = 1e300;
    };
    std::vector<Merged> merged;
};

// The parameters Engine::decide uses that Engine::search does not: a
// decision with other values of these can be replayed on a search
// (sentence-bench -Y). strength too, as long as neither value is Strong
// (Strong's search also weighs the typed word).
bool isDecisionParam(const std::string &name);

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

    // decide(search(taps), params()).
    Result process(const std::vector<Tap> &taps) const;
    // The two halves of process(): the candidates, and what to show and
    // whether to autocorrect, with `params`' decision settings
    // (isDecisionParam; the others must be params()' own).
    Searched search(const std::vector<Tap> &taps) const;
    // (By value: decide() works on its own copy; process() moves.)
    Result decide(Searched searched, const EngineParams &params) const;

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
    // search() in steps (search.cpp).
    void typedWordStatus(Searched &out) const;
    void searchLanguages(Searched &out, bool capital) const;
    void addContext(size_t dict, const Searched &out, std::vector<Candidate> &cands) const;
    void searchSplits(Searched &out, bool capital) const;
    // Extra cost of a word stored in lower case typed with a capital after
    // a word (capitalLowerCost).
    double capitalCost(size_t dict, const std::u32string &stored, bool capital) const;
    // decide() in steps (decide.cpp), on one decision's state.
    struct Decision;
    void unknownReading(Decision &d) const;
    void shares(Decision &d) const;
    bool bar(Decision &d) const;
    bool languageSure(Decision &d) const;
    bool otherLanguageAllows(Decision &d) const;
    bool fixValidWord(Decision &d) const;
    bool fixGentle(Decision &d) const;
    bool fixExpectedCost(Decision &d) const;
    // Folded, without apostrophes: the same letters (piu, più; jai, j'ai).
    std::u32string letters(const std::u32string &word) const;
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
