// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Engine::decide: from the candidates of Engine::search (search.cpp) to the
// suggestion bar and the autocorrection. In order:
//   1. the model's last term, "a word missing from the dictionaries"
//      (unknownReading), and the shares of all readings (shares);
//   2. the suggestion bar and the fix Space would type (bar);
//   3. gates: the language must be sure (languageSure), a word of another
//      typing language is not replaced lightly (otherLanguageAllows);
//   4. the rule: a valid word (fixValidWord: case fixes, Strong's real-word
//      fixes), Gentle's obvious slips (fixGentle), or Normal and Strong's
//      expected cost (fixExpectedCost).
// Each sets Result::reason. The decision's settings may differ from the
// search's (isDecisionParam): tools replay them on one search.

#include "parrotfish/engine.h"
#include "engine-internal.h"

#include <algorithm>
#include <cmath>

namespace parrotfish {

using internal::lower;
using Merged = Searched::Merged;

// One decision's state, shared by its steps.
struct Engine::Decision {
    Decision(const Engine &e, Searched &in, const EngineParams &p) : e(e), in(in), p(p), merged(in.merged) {}
    const Engine &e;
    Searched &in;
    const EngineParams &p;
    std::vector<Merged> &merged;
    Result r;
    std::u32string typedLower, typedFolded, caseFix;
    bool digits = false;
    double unknownProb = 0, unknownProbEqual = 0, total = 0;
    int candidate = -1;          // index of the fix in r.suggestions
    Suggestion best;
    std::u32string bestLower;
    double bestProb = 0;
    bool caseOnly = false;       // a single letter, only its case changed

    // A digit in the word (typed on the number row: h3llo) can only be a
    // slip onto the digit: the fix is the word as typed with a letter in the
    // digit's place. Other words are only offered (any other fix turns names
    // into words: ps4 → is, h2o → who). A letter and a digit is a code
    // (A4, F1, i5): never fixed. Completions are never fixes.
    bool canFix(const Suggestion &s) const
    {
        if (s.completion)
            return false;
        if (!digits)
            return true;
        const std::u32string b = e.fold(s.word);
        if (b.size() != typedFolded.size() || b.size() < 3)
            return false;
        for (size_t i = 0; i < b.size(); ++i)
            if (b[i] != typedFolded[i] && !isDigit(typedFolded[i]))
                return false;
        return true;
    }
    static bool isDigit(char32_t c) { return c >= U'0' && c <= U'9'; }
    // The likeliest complete word.
    const Merged *likeliest() const
    {
        const Merged *out = nullptr;
        for (const Merged &m : merged)
            if (canFix(m.s) && (!out || m.prob > out->prob))
                out = &m;
        return out;
    }
};

std::u32string Engine::letters(const std::u32string &word) const
{
    std::u32string out = fold(word);
    out.erase(std::remove(out.begin(), out.end(), U'\''), out.end());
    return out;
}

Result Engine::decide(Searched in, const EngineParams &params) const
{
    Decision d(*this, in, params);
    Result &r = d.r;
    r.typed = in.typed;
    if (!in.searched)
        return r;
    r.typedIsWord = in.typedIsWord;
    d.caseFix = in.caseFix;
    d.typedLower = lower(r.typed);
    d.typedFolded = fold(r.typed);
    d.digits = std::any_of(r.typed.begin(), r.typed.end(), Decision::isDigit);

    // Letters that are a word only in another case (jaime: the name Jaime,
    // mn: MN) are the user's word with a capital only if that is the
    // likeliest reading; else (j'aime, m'n) the candidates compete as for
    // any other letters.
    if (r.typedIsWord && !in.validAsTyped) {
        const Merged *likeliest = d.likeliest();
        if (likeliest && likeliest->key != d.typedLower) {
            r.typedIsWord = false;
            d.caseFix.clear();
        }
    }

    unknownReading(d);
    shares(d);
    if (!bar(d))
        return std::move(d.r);
    if (!languageSure(d) || !otherLanguageAllows(d))
        return std::move(d.r);

    // A single letter is never replaced, except by itself in another case:
    // English i, stored as a misspelling of I (the only one in the
    // dictionaries). The handler applies it at Space only (i.e. stays).
    d.caseOnly = in.taps.size() == 1 && d.bestLower == d.typedLower;
    const bool apply = r.typedIsWord ? fixValidWord(d)
                       : params.strength == Strength::Gentle ? fixGentle(d)
                                                              : fixExpectedCost(d);
    if (apply) {
        // What Space will type comes first in the bar.
        std::rotate(r.suggestions.begin(), r.suggestions.begin() + d.candidate,
                    r.suggestions.begin() + d.candidate + 1);
        r.autocorrect = 0;
    }
    return std::move(d.r);
}

void Engine::unknownReading(Decision &d) const
{
    // The "intended word missing from the dictionaries" hypothesis: its
    // letters cost by how much they look like a word of the likely
    // languages (Kaito less than xqzt), mixed by the languages' weights as
    // the words are. Not when they are a word's but for accents,
    // apostrophes or spelling rules (garcon, perche, strasse): the letter
    // model knows those letters from that word; nor when they are a word of
    // a less likely language (thier, a German surname, in English text): not
    // a missing word, otherLanguageMargin decides. probEqual: all languages
    // equally likely (the first word).
    if (d.r.typedIsWord)
        return;
    const EngineParams &p = d.p;
    const std::vector<Tap> &taps = d.in.taps;
    const std::vector<double> &w = d.in.weights;
    double tapCost = 0;
    for (const Tap &t : taps) {
        const int k = m_keys.indexOf(t.ch);
        tapCost += k >= 0 ? -std::log(m_keys.probability(k, k)) : 0.0;
    }
    const bool spelling = std::any_of(d.merged.begin(), d.merged.end(), [&](const Merged &m) {
        return d.canFix(m.s) && m.s.changes == 0;
    });
    const bool useLetters = p.unknownLetterWeight > 0 && !spelling && !d.in.wordInSomeLanguage;
    // Long letters one change from a word are mostly that word
    // misspelled (recieve, goverment), short ones are often new words
    // (Ugo, Norn): when the likeliest fix is one word one change away,
    // what the letter model takes off the cost per letter shrinks with
    // each letter past unknownShortLength.
    const Merged *likeliest = d.likeliest();
    const bool nearWord = likeliest && (likeliest->s.changes <= 1
                                        || (likeliest->s.changes == 2 && taps.size() >= p.unknownTwoChangeLength))
                          && likeliest->key.find(U' ') == std::u32string::npos;
    // A compound of dictionary words is a likelier missing word, less so
    // near a word (compoundBonus, compoundNearBonus).
    const double base = p.unknownBase
                        - (d.in.compound ? (nearWord ? p.compoundNearBonus : p.compoundBonus) : 0.0);
    const double flat = p.unknownPerLetter * taps.size();
    const double longer = nearWord && taps.size() > p.unknownShortLength
                          ? p.unknownLongLetterCost * (taps.size() - p.unknownShortLength)
                          : 0.0;
    double mixed = 0, equal = 0;
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        const double letterCost = useLetters ? m_dicts[i]->letterCost(d.typedFolded) : -1.0;
        double cost = flat;
        if (letterCost >= 0) {
            cost = p.unknownLetterWeight * (letterCost - p.unknownLetterRef);
            if (cost < flat)
                cost = std::min(flat, cost + longer);
        }
        const double prob = std::exp(-cost);
        mixed += w[i] * prob;
        equal += prob / m_dicts.size();
    }
    d.unknownProb = std::exp(-(tapCost + base)) * mixed;
    d.unknownProbEqual = std::exp(-(tapCost + base)) * equal;
}

void Engine::shares(Decision &d) const
{
    // Completions don't compete with corrections for confidence: they are
    // words the user may still be typing.
    d.total = d.unknownProb;
    for (Merged &m : d.merged) {
        m.s.cost = -std::log(m.prob);
        if (d.canFix(m.s))
            d.total += m.prob;
    }
    // Calibration: the odds of "a word missing from the dictionaries"
    // against the words swing too far (fit on the typos benches: slope
    // ~0.6), so they are scaled in log space.
    if (d.p.unknownOddsScale != 1.0 && d.unknownProb > 0 && d.total > d.unknownProb) {
        const double words = d.total - d.unknownProb;
        d.unknownProb = words * std::exp(d.p.unknownOddsScale * std::log(d.unknownProb / words));
        d.total = words + d.unknownProb;
    }
    d.r.unknownShare = d.total > 0 ? d.unknownProb / d.total : 0;
    std::sort(d.merged.begin(), d.merged.end(),
              [](const Merged &a, const Merged &b) { return a.prob > b.prob; });
}

bool Engine::bar(Decision &d) const
{
    Result &r = d.r;
    for (Merged &m : d.merged) {
        if ((m.s.attrs & format::AttrOffensive) && lower(m.s.word) != d.typedLower)
            continue;   // never offered unprompted
        m.s.word = applyCase(m.s.word, d.in.taps);
        if (m.s.word == r.typed)
            continue;
        r.suggestions.push_back(m.s);
    }
    if (!d.caseFix.empty()) {
        auto it = std::find_if(r.suggestions.begin(), r.suggestions.end(),
                               [&](const Suggestion &s) { return s.word == d.caseFix; });
        if (it != r.suggestions.end())
            std::rotate(r.suggestions.begin(), it, it + 1);
    }
    // Autocorrect considers complete words only: at Space the word is
    // finished, so the candidate is the best suggestion of the typed length.
    for (size_t i = 0; i < r.suggestions.size() && d.candidate < 0; ++i)
        if (d.canFix(r.suggestions[i]))
            d.candidate = static_cast<int>(i);
    if (d.candidate < 0) {
        r.reason = d.digits ? Reason::Digits : Reason::NoCandidate;
        return false;
    }
    d.best = r.suggestions[d.candidate];

    // Its share of the probability of all complete words and of "a word
    // missing from the dictionaries".
    d.bestLower = lower(d.best.word);
    for (const Merged &m : d.merged)
        if (m.key == d.bestLower)
            d.bestProb += m.prob;
    r.confidence = d.total > 0 ? d.bestProb / d.total : 0;
    return true;
}

bool Engine::languageSure(Decision &d) const
{
    const EngineParams &p = d.p;
    const std::vector<double> &w = d.in.weights;
    double bestWeight = 0;
    for (size_t i = 0; i < m_dicts.size(); ++i)
        if (d.best.languages & (1u << i))
            bestWeight = std::max(bestWeight, w[i]);
    bool sure = bestWeight >= p.autocorrectConfidence;
    if (!sure && d.in.textWords == 0 && bestWeight >= p.firstWordConfidence) {
        // The first word: the same correction whichever language the text
        // is in, and sure enough with the lean and without it.
        double totalEqual = d.unknownProbEqual, bestEqual = 0, topEqual = 0;
        for (const Merged &m : d.merged) {
            if (m.s.completion)
                continue;
            totalEqual += m.probEqual;
            topEqual = std::max(topEqual, m.probEqual);
            if (m.key == d.bestLower)
                bestEqual = m.probEqual;
        }
        const double shareEqual = totalEqual > 0 ? bestEqual / totalEqual : 0;
        sure = bestEqual >= topEqual && std::min(d.r.confidence, shareEqual) >= p.firstWordShare;
    }
    const bool offensive = d.best.attrs & format::AttrOffensive;
    if (sure && !offensive && p.strength != Strength::Off)
        return true;
    d.r.reason = !sure ? Reason::LanguageNotSure : offensive ? Reason::Offensive : Reason::StrengthOff;
    return false;
}

bool Engine::otherLanguageAllows(Decision &d) const
{
    // A word of a typing language that seems unlikely in the text (the
    // first words after a switch: Italian "Mi" after English text) is
    // replaced by a different word only if the fix is otherLanguageMargin
    // nats likelier than the word as typed, read as a word of its language
    // with that language's weight. Accents and apostrophes may always be
    // fixed (fur → für in German text, English jai → j'ai in French text).
    const Result &r = d.r;
    if (!d.in.wordInSomeLanguage || r.typedIsWord || letters(d.best.word) == letters(r.typed))
        return true;
    const EngineParams &p = d.p;
    const std::vector<double> &w = d.in.weights;
    if (!std::isfinite(p.otherLanguageMargin)) {
        d.r.reason = Reason::OtherLanguage;
        return false;
    }
    double typedProb = 0;
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        std::u32string stored;
        const uint32_t node = m_dicts[i]->find(r.typed, &stored);
        if (!node || (m_dicts[i]->attributes(node) & format::AttrMisspelling) || w[i] <= 0)
            continue;
        // A name typed in lower case (suess: Suess) is not that word.
        if (stored != r.typed && lower(stored) == d.typedLower && applyCase(stored, d.in.taps) != r.typed)
            continue;
        const double tapCost = m_correctors[i]->matchCost(d.in.taps, stored);
        if (std::isfinite(tapCost))
            typedProb += std::exp(-(tapCost + p.corrector.lmWeight * -m_dicts[i]->logProb(
                                        m_dicts[i]->node(node).freq) - std::log(w[i])));
    }
    if (typedProb > 0 && std::log(d.bestProb / typedProb) < p.otherLanguageMargin) {
        d.r.reason = Reason::OtherLanguage;
        return false;
    }
    return true;
}

bool Engine::fixValidWord(Decision &d) const
{
    // Only a capitalisation fix (haus → Haus) replaces a valid word, and
    // with Strong a much likelier word one slip away (after a word), but
    // not a word that also stands for another (id: I'd, ill: I'll).
    Result &r = d.r;
    const EngineParams &p = d.p;
    const Searched &in = d.in;
    bool apply = !d.caseFix.empty() && d.best.word == d.caseFix;
    r.reason = apply ? Reason::CaseFix : Reason::TypedIsWord;
    if (apply || p.strength != Strength::Strong || in.typedHasShortcut || in.prev.empty() || in.sentenceStart
            || in.taps.size() < p.strongMinLength || letters(d.best.word) == letters(r.typed)
            || !(neighbourSwap(r.typed, d.best.word) || swapped(r.typed, d.best.word)))
        return apply;
    // A slip within one language: both words must be at home in one typing
    // language, within strongHomeMargin nats of their likeliest language.
    // A word of another language in the text is not a slip (Italian con in
    // German text: von; bene in English text: been); the dictionaries also
    // hold other languages' words, rarer, so being listed is not enough.
    const auto logProbs = [&](const std::u32string &word) {
        std::vector<double> lp(m_dicts.size(), -INFINITY);
        for (size_t i = 0; i < m_dicts.size(); ++i) {
            const uint32_t node = m_dicts[i]->find(word);
            if (node && !(m_dicts[i]->attributes(node) & format::AttrMisspelling) && in.weights[i] > 0)
                lp[i] = m_dicts[i]->logProb(m_dicts[i]->node(node).freq);
        }
        return lp;
    };
    const std::vector<double> typedLp = logProbs(r.typed), bestLp = logProbs(d.best.word);
    const double typedTop = *std::max_element(typedLp.begin(), typedLp.end());
    const double bestTop = *std::max_element(bestLp.begin(), bestLp.end());
    bool sameLanguage = false;
    for (size_t i = 0; i < m_dicts.size() && !sameLanguage; ++i)
        sameLanguage = std::isfinite(typedLp[i]) && std::isfinite(bestLp[i])
                && typedLp[i] >= typedTop - p.strongHomeMargin && bestLp[i] >= bestTop - p.strongHomeMargin;
    if (!sameLanguage) {
        r.reason = Reason::StrongLanguages;
        return apply;
    }
    double typedProb = 0;
    for (const Merged &m : d.merged)
        if (m.key == d.typedLower)
            typedProb += m.prob;
    // A word that is mostly a slip: only with the previous word's support
    // (see Searched::typedHasSlipShortcut).
    bool supported = !in.typedHasSlipShortcut;
    for (size_t i = 0; i < m_dicts.size() && !supported; ++i) {
        if (!(d.best.languages & (1u << i)))
            continue;
        const uint32_t prevNode = m_dicts[i]->find(in.prev);
        if (!prevNode)
            continue;
        for (const auto &n : m_dicts[i]->nextWords(prevNode, p.pairSupportRank))
            if (lower(n.first) == d.bestLower)
                supported = true;
    }
    r.reason = supported ? Reason::StrongMargin : Reason::SlipShortcut;
    if (supported && typedProb > 0 && d.bestProb > 0) {
        r.margin = std::log(d.bestProb / typedProb);
        apply = r.realWordFix = r.margin >= p.strongMargin;
        if (apply)
            r.reason = Reason::Applied;
    }
    return apply;
}

bool Engine::fixGentle(Decision &d) const
{
    // Gentle: only obvious slips, at most one edit and few letters changed.
    Result &r = d.r;
    const EngineParams &p = d.p;
    const size_t n = d.in.taps.size();
    const int maxChanges = n <= p.gentleShortLength ? 1 : 2;
    bool apply = r.confidence >= p.gentleConfidence && d.best.edits <= 1
                 && d.best.changes <= maxChanges && (n >= p.gentleMinLength || d.caseOnly);
    // Two letters: a neighbouring key, or only an accent (ca → ça on a
    // layout without ç), with a higher confidence.
    if (!apply && n == 2 && r.confidence >= p.gentleTwoLetterConfidence)
        apply = neighbourSwap(r.typed, d.best.word)
                || (p.gentleTwoLetterAccents && fold(d.best.word) == fold(r.typed));
    r.reason = apply ? Reason::Applied
               : r.confidence < p.gentleConfidence ? Reason::BelowGentle
               : n < p.gentleMinLength && !d.caseOnly ? Reason::TooShort
                                                      : Reason::GentleChanges;
    return apply;
}

bool Engine::fixExpectedCost(Decision &d) const
{
    // Normal and Strong: the expected loss of the fix against that of
    // leaving the letters (EngineParams::changeCost). Short words have more
    // neighbours and are more often names; a fix that only adds accents or
    // apostrophes (piu: più, jai: j'ai) is neither, whatever its length.
    Result &r = d.r;
    const EngineParams &p = d.p;
    const size_t n = d.in.taps.size();
    const bool sameLetters = letters(d.best.word) == letters(r.typed);
    const double change = sameLetters ? p.changeCost
                          : n <= 2 && p.changeCostTwo > 0 ? p.changeCostTwo
                          : n <= 4 && p.changeCostShort > 0 ? p.changeCostShort
                                                             : p.changeCost;
    const double other = std::max(0.0, 1.0 - r.confidence - r.unknownShare);
    // Another word meant: a wrong word typed instead of the slip, unless
    // it is the same word but for its ending (receive, received;
    // assassino, assassini): nearer to it than the slip was.
    double near = 0;
    if (p.nearWrongCost != p.wrongCost && d.total > 0) {
        auto nearTo = [](const std::u32string &a, const std::u32string &b) {
            if (a.size() < 5 || b.size() < 5 || (a.size() > b.size() ? a.size() - b.size() : b.size() - a.size()) > 2)
                return false;
            const size_t stem = std::min(a.size(), b.size()) - 2;
            return a.compare(0, stem, b, 0, stem) == 0;
        };
        for (const Merged &m : d.merged)
            if (d.canFix(m.s) && m.key != d.bestLower && nearTo(m.key, d.bestLower))
                near += m.prob / d.total;
        near = std::min(near, other);
    }
    const bool longEnough = n >= p.normalMinLength || d.caseOnly;
    const bool apply = r.confidence > r.unknownShare * change + (other - near) * (p.wrongCost - 1.0)
                                          + near * (p.nearWrongCost - 1.0)
                       && longEnough;
    r.reason = apply ? Reason::Applied : !longEnough ? Reason::TooShort : Reason::BelowNormal;
    return apply;
}

} // namespace parrotfish
