// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Engine::search: the candidates for the taps of one word, from every
// language, with their probabilities (the noisy channel: taps, word
// frequency, the previous word). Engine::decide (decide.cpp) then reads
// them; nothing here decides whether to autocorrect.

#include "parrotfish/engine.h"
#include "engine-internal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace parrotfish {

using internal::lower;

namespace {

// Adds a candidate to the merged list: spellings that differ only in case
// (zweiten, Zweiten) are one suggestion, shown in its most likely spelling.
// probEqual: the probability with all languages equally likely.
void merge(std::vector<Searched::Merged> &merged, const std::u32string &word, double cost, double weight,
           size_t dicts, size_t dict, uint32_t attrs, int edits, int changes, bool completion)
{
    const std::u32string key = lower(word);
    auto it = std::find_if(merged.begin(), merged.end(), [&](const Searched::Merged &m) { return m.key == key; });
    if (it == merged.end()) {
        merged.push_back({});
        it = merged.end() - 1;
        it->key = key;
    }
    it->prob += std::exp(-cost);
    it->probEqual += std::exp(-cost) / (weight * dicts);
    it->s.languages |= 1u << dict;
    if (cost < it->bestCost) {
        it->bestCost = cost;
        it->s.word = word;
        it->s.attrs = attrs;
        it->s.edits = edits;
        it->s.changes = changes;
        it->s.completion = completion;
    }
}

} // namespace

Searched Engine::search(const std::vector<Tap> &taps) const
{
    Searched out;
    out.taps = taps;
    for (const Tap &t : taps)
        out.typed.push_back(t.upper ? utf8::toUpper(t.ch) : t.ch);
    if (taps.empty() || m_dicts.empty() || taps.size() > internal::MaxTaps)
        return out;
    out.searched = true;
    out.weights = m_tracker.weights();
    out.sentenceStart = m_context.sentenceStart;
    out.prev = m_context.prev;
    out.textWords = m_textWords;

    typedWordStatus(out);
    // A capital after a word (not all capitals): a name, or a word stored
    // with its capital, rarely a word stored in lower case.
    const bool capital = !m_context.sentenceStart && !m_context.prev.empty() && taps[0].upper
                         && !std::all_of(taps.begin(), taps.end(), [](const Tap &t) { return t.upper; });
    searchLanguages(out, capital);
    searchSplits(out, capital);
    return out;
}

void Engine::typedWordStatus(Searched &out) const
{
    // Is the typed word valid in a likely language? In any typing language?
    const std::vector<double> &w = out.weights;
    const std::u32string typedLower = lower(out.typed);
    // id: also I'd, the same letters with an apostrophe (Strong leaves it
    // alone). Other shortcuts (AOSP's list of words that are mostly slips:
    // foe for for, cam for can) let Strong fix the word only where the
    // previous word is known to be followed by the fix (thanks foe: for;
    // my worst foe stays): the word-pair data keeps a word's commonest
    // followers only, so without that, frequency alone would decide.
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        std::u32string stored;
        uint32_t node = m_dicts[i]->find(out.typed, &stored);
        // No exact-case match: the lowercase spelling, if the typed
        // capitals make it the typed word (Ok at a sentence start is ok
        // with its capital, not OK in another case).
        if (node && stored != out.typed && stored != typedLower) {
            const uint32_t lowerNode = m_dicts[i]->findExact(typedLower);
            if (lowerNode && !(m_dicts[i]->attributes(lowerNode) & format::AttrMisspelling)
                && applyCase(typedLower, out.taps) == out.typed) {
                node = lowerNode;
                stored = typedLower;
            }
        }
        if (!node && w[i] >= m_params.validThreshold && m_dicts[i]->isCompound(typedLower))
            out.compound = true;
        if (node ? (m_dicts[i]->attributes(node) & format::AttrMisspelling)
                 : !m_dicts[i]->isWordForm(typedLower))
            continue;
        out.wordInSomeLanguage = true;
        if (w[i] < m_params.validThreshold)
            continue;
        out.typedIsWord = true;
        std::u32string target;
        if (node && (m_dicts[i]->attributes(node) & format::AttrShorthand))
            out.typedShorthand = true;
        if (node && (m_dicts[i]->attributes(node, &target) & format::AttrShortcut)) {
            std::u32string a = lower(target), b = typedLower;
            a.erase(std::remove(a.begin(), a.end(), U'\''), a.end());
            if (a == b)
                out.typedHasShortcut = true;
            else
                out.typedHasSlipShortcut = true;
        }
        // Not only in another case (jaime: Jaime only); the stored
        // capitalisation, if it differs (haus → Haus).
        const bool otherCase = node && stored != out.typed && lower(stored) == typedLower
                               && applyCase(stored, out.taps) != out.typed;
        if (!otherCase)
            out.validAsTyped = true;
        else if (out.caseFix.empty())
            out.caseFix = applyCase(stored, out.taps);
    }
    // A learned new word is a word of the language it was learned in.
    for (size_t i = 0; i < m_dicts.size() && m_learned; ++i) {
        const LearnedCache &lc = learnedCache(i);
        const Learned::Word *lw = lc.lang ? lc.lang->word(typedLower) : nullptr;
        if (!lw || !lw->isNew)
            continue;
        out.wordInSomeLanguage = true;
        if (w[i] < m_params.validThreshold)
            continue;
        out.typedIsWord = true;
        const bool otherCase = lw->spelling != out.typed && applyCase(lw->spelling, out.taps) != out.typed;
        if (!otherCase)
            out.validAsTyped = true;
        else if (out.caseFix.empty())
            out.caseFix = applyCase(lw->spelling, out.taps);
    }
}

// P(next | prev) of a stored next word.
static double pairFreqProb(uint8_t f)
{
    return std::exp((static_cast<int>(f) - 255) * format::LogStep);
}

double Engine::withContext(double pair, double uni) const
{
    return m_params.pairWeight * pair + (1.0 - m_params.pairWeight) * uni;
}

double Engine::capitalCost(size_t dict, const std::u32string &stored, bool capital) const
{
    if (!capital || stored.empty() || utf8::isUpper(stored[0]))
        return 0.0;
    return m_dicts[dict]->capitalisedNouns() ? m_params.capitalLowerCostNouns : m_params.capitalLowerCost;
}

void Engine::searchLanguages(Searched &out, bool capital) const
{
    // Search every language, weighted by its probability, and merge.
    const std::vector<double> &w = out.weights;
    const bool useContext = m_params.pairWeight > 0 && !m_context.insideWord;
    // Most likely language first, so the others can be cut short.
    std::vector<size_t> order(m_dicts.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return w[a] > w[b]; });
    const bool prune = m_params.searchMargin > 0;
    double bestCost = std::numeric_limits<double>::infinity();
    for (size_t i : order) {
        if (w[i] <= m_params.searchMinWeight)
            continue;
        std::vector<Candidate> cands = m_correctors[i]->search(
            out.taps, std::log(w[i]), prune ? bestCost + m_params.searchMargin
                                            : std::numeric_limits<double>::infinity());
        if (useContext || learnedCache(i).lang)
            addContext(i, out, cands, useContext);

        for (Candidate &c : cands) {
            // A listed misspelling typed with a capital after a word is not
            // the habit (cant for can't) but likely a name (Mr Cant): no
            // bonus for its target.
            const double capitalExtra = capitalCost(i, c.word, capital);
            c.cost += capitalExtra;
            if (capitalExtra > 0 && c.viaTarget && c.changes == 0 && (c.attrs & format::AttrMisspelling))
                c.cost += m_params.corrector.exactMisspellingBonus;
            bestCost = std::min(bestCost, c.cost);
            merge(out.merged, c.word, c.cost, w[i], m_dicts.size(), i, c.attrs, c.edits, c.changes,
                  c.extraLetters > 0);
        }
    }
}

const Engine::ContextNext &Engine::contextNext(size_t i) const
{
    if (m_contextNext.size() != m_dicts.size())
        m_contextNext.assign(m_dicts.size(), ContextNext());
    ContextNext &c = m_contextNext[i];
    const uint64_t version = m_learned ? m_learned->version() : 0;
    if (c.valid && c.sentenceStart == m_context.sentenceStart && c.prev == m_context.prev
            && c.learnedVersion == version)
        return c;
    c = ContextNext();
    c.valid = true;
    c.learnedVersion = version;
    c.sentenceStart = m_context.sentenceStart;
    c.prev = m_context.prev;
    std::vector<std::pair<std::u32string, double>> next = nextWords(i, m_context.sentenceStart, m_context.prev, false);
    // One lookup per candidate, however many next words are stored.
    c.byKey.reserve(next.size());
    c.words.reserve(next.size());
    for (auto &n : next) {
        c.byKey.emplace(lower(n.first), n.second);
        c.words.push_back(std::move(n.first));
    }
    return c;
}

void Engine::addContext(size_t i, const Searched &out, std::vector<Candidate> &cands, bool useContext) const
{
    // The previous word: P(word) = λ P(word | prev) + (1 - λ) P(word), with
    // the user's use of the word blended into P(word) (learnedUni) and of
    // the pair into P(word | prev) (nextWords).
    const Dictionary &d = *m_dicts[i];
    const std::vector<double> &w = out.weights;
    const LearnedCache &lc = learnedCache(i);
    static const ContextNext none;
    const ContextNext &cn = useContext ? contextNext(i) : none;
    auto pairProb = [&](const std::u32string &key) {
        const auto it = cn.byKey.find(key);
        return it == cn.byKey.end() ? 0.0 : it->second;
    };
    auto mixed = [&](const std::u32string &key, double dictUni) {
        const double uni = learnedUni(lc, key, dictUni);
        return useContext ? withContext(pairProb(key), uni) : uni;
    };
    auto rescore = [&](Candidate &c, const std::u32string &key) {
        const double uni = std::exp(d.logProb(c.freq));
        const double delta = m_params.corrector.lmWeight * (std::log(mixed(key, uni)) - std::log(uni));
        c.cost -= delta;
        c.wordCost -= delta;
    };
    std::unordered_set<std::u32string> keys;   // the candidates', lowercase
    for (Candidate &c : cands) {
        std::u32string key = lower(c.word);
        rescore(c, key);
        keys.insert(std::move(key));
    }
    auto have = [&](const std::u32string &key) { return keys.count(key) > 0; };
    // A word the search didn't reach (it ranks by P(word)).
    auto reach = [&](const std::u32string &word) {
        const std::u32string key = lower(word);
        if (have(key))
            return;
        if (lc.lang) {
            const Learned::Word *lw = lc.lang->word(key);
            if (lw && lw->isNew)
                return;   // a learned word: below
        }
        int edits = 0, changes = 0;
        const double tapCost = m_correctors[i]->matchCost(out.taps, word, &edits, &changes);
        if (!std::isfinite(tapCost))
            return;
        Candidate c;
        c.word = word;
        const uint32_t node = d.findExact(word);
        c.freq = node ? d.node(node).freq : 0;
        c.attrs = node ? d.attributes(node) : 0;
        c.edits = edits;
        c.changes = changes;
        c.tapCost = tapCost;
        c.wordCost = -(m_params.corrector.lmWeight * d.logProb(c.freq) + std::log(w[i]));
        c.cost = tapCost + c.wordCost;
        rescore(c, key);
        cands.push_back(c);
        keys.insert(key);
    };
    if (useContext) {
        for (size_t k = 0; k < cn.words.size() && k < m_params.pairReach; ++k)
            reach(cn.words[k]);   // likely next words
        // Strong weighs the typed word against its fixes: it must be there.
        if (m_params.strength == Strength::Strong && out.typedIsWord) {
            std::u32string stored;
            const uint32_t node = d.find(out.typed, &stored);
            if (node && !(d.attributes(node) & format::AttrMisspelling))
                reach(stored);
        }
    }
    // The learned words: new ones, and dictionary words the user makes
    // much likelier (they may be past the corrector's best few).
    if (lc.reach.empty())
        return;
    const std::vector<Corrector::WordMatch> matches = m_correctors[i]->matchLetters(out.taps, lc.reachLetters);
    std::vector<Candidate> learnedCands;
    for (size_t k = 0; k < lc.reach.size(); ++k) {
        const Corrector::WordMatch &m = matches[k];
        const LearnedCache::Reach &r = lc.reach[k];
        if (!std::isfinite(m.tapCost) || have(r.key))
            continue;
        Candidate c;
        c.word = r.word;
        c.edits = m.edits;
        c.changes = m.changes;
        c.extraLetters = m.extraLetters;
        c.tapCost = m.tapCost;
        c.wordCost = -(m_params.corrector.lmWeight * std::log(mixed(r.key, 0.0)) + std::log(w[i]));
        c.cost = c.tapCost + c.wordCost;
        if (std::isfinite(c.cost))
            learnedCands.push_back(std::move(c));
    }
    // As many as the corrector gives, the best (after one letter, every
    // learned word starting with it is a completion).
    const size_t keep = std::min(learnedCands.size(), m_params.corrector.maxResults);
    std::partial_sort(learnedCands.begin(), learnedCands.begin() + static_cast<long>(keep), learnedCands.end(),
                      [](const Candidate &a, const Candidate &b) { return a.cost < b.cost; });
    for (size_t k = 0; k < keep; ++k)
        cands.push_back(std::move(learnedCands[k]));
}

void Engine::searchSplits(Searched &out, bool capital) const
{
    // Two words typed as one: the space left out (thisis) or typed as a key
    // above the spacebar (thisbis). Both words of one language, each as
    // typed; never for a word of a typing language (compounds stay whole).
    if (out.wordInSomeLanguage || (!std::isfinite(m_params.missingSpaceCost) && !std::isfinite(m_params.spaceKeyCost)))
        return;
    const std::vector<Tap> &taps = out.taps;
    const std::vector<double> &w = out.weights;
    const bool useContext = m_params.pairWeight > 0 && !m_context.insideWord;
    const size_t n = taps.size(), minLen = std::max<size_t>(1, m_params.splitMinLength);
    // The bottom letter row: its keys border the spacebar.
    float bottom = -1e30f;
    for (const Key &k : m_keys.keys())
        bottom = std::max(bottom, k.y);
    auto aboveSpace = [&](char32_t c) {
        const int k = m_keys.indexOf(c);
        return k >= 0 && m_keys.keys()[k].y > bottom - m_keys.keys()[k].height / 2;
    };
    // log P(word), or with the word before it as context.
    auto logProb = [&](const Dictionary &d, uint32_t node, const std::u32string &word, uint32_t prevNode) {
        const double uni = d.logProb(d.node(node).freq);
        if (!useContext || !prevNode)
            return uni;
        double pair = 0;
        const std::u32string key = lower(word);
        for (const auto &nw : d.nextWords(prevNode))
            if (lower(nw.first) == key) {
                pair = pairFreqProb(nw.second);
                break;
            }
        return std::log(withContext(pair, std::exp(uni)));
    };
    auto trySplit = [&](size_t leftEnd, size_t rightStart, double extra) {
        const std::vector<Tap> lt(taps.begin(), taps.begin() + leftEnd), rt(taps.begin() + rightStart, taps.end());
        std::u32string left, right;
        for (const Tap &t : lt) left.push_back(t.ch);
        for (const Tap &t : rt) right.push_back(t.ch);
        for (size_t i = 0; i < m_dicts.size(); ++i) {
            if (w[i] <= m_params.searchMinWeight)
                continue;
            const Dictionary &d = *m_dicts[i];
            std::u32string ls, rs;
            const uint32_t ln = d.find(left, &ls), rn = d.find(right, &rs);
            if (!ln || !rn || (d.attributes(ln) & format::AttrMisspelling)
                    || (d.attributes(rn) & format::AttrMisspelling))
                continue;
            const double tapCost = m_correctors[i]->matchCost(lt, ls) + m_correctors[i]->matchCost(rt, rs);
            if (!std::isfinite(tapCost))
                continue;
            const uint32_t prev = m_context.prev.empty() || m_context.sentenceStart ? 0 : d.find(m_context.prev);
            const double cost = tapCost + extra - std::log(w[i]) + capitalCost(i, ls, capital)
                    - m_params.corrector.lmWeight * (logProb(d, ln, ls, prev) + logProb(d, rn, rs, ln));
            merge(out.merged, ls + U' ' + rs, cost, w[i], m_dicts.size(), i,
                  d.attributes(ln) | d.attributes(rn), 1, 1, false);
        }
    };
    for (size_t k = minLen; k + minLen <= n; ++k) {
        if (std::isfinite(m_params.missingSpaceCost))
            trySplit(k, k, m_params.missingSpaceCost);
        if (std::isfinite(m_params.spaceKeyCost) && k + 1 + minLen <= n && aboveSpace(taps[k].ch))
            trySplit(k, k + 1, m_params.spaceKeyCost);
    }
}

} // namespace parrotfish
