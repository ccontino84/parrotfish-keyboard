// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <cmath>

namespace parrotfish {

namespace {

// Longer input (a pasted string, a URL) is left as typed.
constexpr size_t MaxTaps = 40;

bool isWordChar(char32_t c)
{
    return utf8::toLower(c) != utf8::toUpper(c) || c == U'ß'
        || (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7);
}

std::u32string lower(std::u32string s)
{
    for (char32_t &c : s)
        c = utf8::toLower(c);
    return s;
}

} // namespace

Engine::Engine(EngineParams params)
    : m_params(params)
{
}

Engine::~Engine() = default;

bool Engine::addLanguage(const std::string &dictPath, std::string *error)
{
    auto d = std::make_unique<Dictionary>();
    if (!d->open(dictPath, error))
        return false;
    m_dicts.push_back(std::move(d));
    rebuild();
    return true;
}

void Engine::clearLanguages()
{
    m_dicts.clear();
    rebuild();
}

void Engine::setParams(const EngineParams &params)
{
    m_params = params;
    rebuild();
}

int Engine::languageIndex(const std::string &code) const
{
    for (size_t i = 0; i < m_dicts.size(); ++i)
        if (m_dicts[i]->language() == code)
            return static_cast<int>(i);
    return -1;
}

void Engine::setLayout(const KeyModel &keys)
{
    m_layout = keys.keys();
    m_layoutSigma = keys.sigma();
    rebuild();
}

std::u32string Engine::fold(const std::u32string &word) const
{
    std::u32string out;
    for (char32_t c : word) {
        const char32_t l = utf8::toLower(c);
        auto it = m_base.find(l);
        out.push_back(it == m_base.end() ? l : it->second);
    }
    return out;
}

void Engine::rebuild()
{
    m_correctors.clear();
    m_keys = KeyModel(m_layout, m_layoutSigma, static_cast<float>(m_params.keyHeightShare));
    m_base.clear();
    for (const auto &d : m_dicts)
        for (uint32_t i = 0; i < d->alphabetSize(); ++i)
            m_base[d->letter(i).lower] = d->letter(i).base;
    std::vector<const Dictionary *> dicts;
    for (const auto &d : m_dicts) {
        m_correctors.push_back(std::make_unique<Corrector>(*d, m_keys, m_params.corrector));
        dicts.push_back(d.get());
    }
    m_tracker = LanguageTracker(dicts, m_params.language);
}

Engine::Context Engine::parseContext(const std::u32string &textBefore)
{
    Context ctx;
    size_t end = textBefore.size();
    bool sawSpace = false;
    while (end > 0) {
        const char32_t c = textBefore[end - 1];
        if (isWordChar(c) || c == U'\'' || c == U'-')
            break;
        if (c == U'.' || c == U'!' || c == U'?' || c == U'\n' || c == U'…')
            return ctx;   // a new sentence
        sawSpace = true;
        --end;
    }
    if (end == 0)
        return ctx;
    ctx.sentenceStart = false;
    ctx.insideWord = !sawSpace;
    const std::vector<std::u32string> ws = words(textBefore.substr(0, end));
    if (!ws.empty())
        ctx.prev = ws.back();
    return ctx;
}

void Engine::startText(const std::u32string &textBefore, const std::vector<double> &prior)
{
    m_context = parseContext(textBefore);
    m_tracker.reset(prior);
    const std::vector<std::u32string> ws = words(textBefore);
    m_textWords = ws.size();
    for (const std::u32string &w : ws)
        m_tracker.observe(w);
}

void Engine::commit(const std::u32string &word)
{
    ++m_textWords;
    m_tracker.observe(word);
}

Result Engine::process(const std::vector<Tap> &taps) const
{
    return decide(search(taps), m_params);
}

Searched Engine::search(const std::vector<Tap> &taps) const
{
    Searched out;
    out.taps = taps;
    Searched &r = out;   // (the fields process() used to fill)
    for (const Tap &t : taps)
        r.typed.push_back(t.upper ? utf8::toUpper(t.ch) : t.ch);
    if (taps.empty() || m_dicts.empty() || taps.size() > MaxTaps)
        return out;
    out.searched = true;
    out.weights = m_tracker.weights();
    out.sentenceStart = m_context.sentenceStart;
    out.prev = m_context.prev;
    out.textWords = m_textWords;
    const std::vector<double> &w = out.weights;

    // Is the typed word valid in a likely language? In any typing language?
    const std::u32string typedLower = lower(r.typed);
    std::u32string &caseFix = out.caseFix;   // the stored capitalisation, if it differs (haus → Haus)
    bool &wordInSomeLanguage = out.wordInSomeLanguage;
    // id: also I'd, the same letters with an apostrophe (Strong leaves it
    // alone). Other shortcuts (AOSP's list of words that are mostly slips:
    // foe for for, cam for can) let Strong fix the word only where the
    // previous word is known to be followed by the fix (thanks foe: for;
    // my worst foe stays): the word-pair data keeps a word's commonest
    // followers only, so without that, frequency alone would decide.
    bool &typedHasShortcut = out.typedHasShortcut, &typedHasSlipShortcut = out.typedHasSlipShortcut;
    bool &validAsTyped = out.validAsTyped;   // not only in another case (jaime: Jaime only)
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        std::u32string stored;
        const uint32_t node = m_dicts[i]->find(r.typed, &stored);
        if (node ? (m_dicts[i]->attributes(node) & format::AttrMisspelling)
                 : !m_dicts[i]->isWordForm(typedLower))
            continue;
        wordInSomeLanguage = true;
        if (w[i] < m_params.validThreshold)
            continue;
        r.typedIsWord = true;
        std::u32string target;
        if (node && (m_dicts[i]->attributes(node, &target) & format::AttrShortcut)) {
            std::u32string a = lower(target), b = typedLower;
            a.erase(std::remove(a.begin(), a.end(), U'\''), a.end());
            if (a == b)
                typedHasShortcut = true;
            else
                typedHasSlipShortcut = true;
        }
        const bool otherCase = node && stored != r.typed && lower(stored) == typedLower
                               && applyCase(stored, taps) != r.typed;
        if (!otherCase)
            validAsTyped = true;
        else if (caseFix.empty())
            caseFix = applyCase(stored, taps);
    }

    // A capital after a word (not all capitals): a name, or a word stored
    // with its capital, rarely a word stored in lower case.
    const bool capital = !m_context.sentenceStart && !m_context.prev.empty() && taps[0].upper
                         && !std::all_of(taps.begin(), taps.end(), [](const Tap &t) { return t.upper; });
    auto capitalCost = [&](size_t dict, const std::u32string &stored) {
        if (!capital || stored.empty() || utf8::isUpper(stored[0]))
            return 0.0;
        return m_dicts[dict]->language() == "de" ? m_params.capitalLowerCostNouns : m_params.capitalLowerCost;
    };

    // Search every language, weighted by its probability, and merge.
    // Spellings that differ only in case (zweiten, Zweiten) are one
    // suggestion, shown in its most likely spelling.
    // probEqual: the probability with all languages equally likely.
    using Merged = Searched::Merged;
    std::vector<Merged> &merged = out.merged;
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
        const Dictionary &d = *m_dicts[i];
        std::vector<Candidate> cands = m_correctors[i]->search(
            taps, std::log(w[i]), prune ? bestCost + m_params.searchMargin
                                        : std::numeric_limits<double>::infinity());

        // The previous word: P(word) = λ P(word | prev) + (1 - λ) P(word).
        if (useContext) {
            std::vector<std::pair<std::u32string, uint8_t>> next;
            if (m_context.sentenceStart) {
                next = d.nextWords(format::PairStart);
            } else if (!m_context.prev.empty()) {
                const uint32_t node = d.find(m_context.prev);
                if (node && !(d.attributes(node) & format::AttrMisspelling))
                    next = d.nextWords(node);
            }
            const double lambda = m_params.pairWeight;
            auto pairProb = [&](const std::u32string &word) {
                const std::u32string key = lower(word);
                for (const auto &n : next)
                    if (lower(n.first) == key)
                        return std::exp((static_cast<int>(n.second) - 255) * format::LogStep);
                return 0.0;
            };
            auto rescore = [&](Candidate &c) {
                const double uni = std::exp(d.logProb(c.freq));
                const double mixed = lambda * pairProb(c.word) + (1.0 - lambda) * uni;
                const double delta = m_params.corrector.lmWeight * (std::log(mixed) - std::log(uni));
                c.cost -= delta;
                c.wordCost -= delta;
            };
            for (Candidate &c : cands)
                rescore(c);
            // A word the search didn't reach (it ranks by P(word)).
            auto reach = [&](const std::u32string &word) {
                const std::u32string key = lower(word);
                if (std::any_of(cands.begin(), cands.end(),
                                [&](const Candidate &c) { return lower(c.word) == key; }))
                    return;
                int edits = 0, changes = 0;
                const double tapCost = m_correctors[i]->matchCost(taps, word, &edits, &changes);
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
                rescore(c);
                cands.push_back(c);
            };
            for (const auto &n : next)
                reach(n.first);   // likely next words
            // Strong weighs the typed word against its fixes: it must be there.
            if (m_params.strength == Strength::Strong && r.typedIsWord) {
                std::u32string stored;
                const uint32_t node = d.find(r.typed, &stored);
                if (node && !(d.attributes(node) & format::AttrMisspelling))
                    reach(stored);
            }
        }

        for (Candidate &c : cands) {
            // A listed misspelling typed with a capital after a word is not
            // the habit (cant for can't) but likely a name (Mr Cant): no
            // bonus for its target.
            const double capitalExtra = capitalCost(i, c.word);
            c.cost += capitalExtra;
            if (capitalExtra > 0 && c.viaTarget && c.changes == 0 && (c.attrs & format::AttrMisspelling))
                c.cost += m_params.corrector.exactMisspellingBonus;
            bestCost = std::min(bestCost, c.cost);
            const std::u32string key = lower(c.word);
            auto it = std::find_if(merged.begin(), merged.end(),
                                   [&](const Merged &m) { return m.key == key; });
            if (it == merged.end()) {
                merged.push_back({});
                it = merged.end() - 1;
                it->key = key;
            }
            it->prob += std::exp(-c.cost);
            it->probEqual += std::exp(-c.cost) / (w[i] * m_dicts.size());
            it->s.languages |= 1u << i;
            if (c.cost < it->bestCost) {
                it->bestCost = c.cost;
                it->s.word = c.word;
                it->s.attrs = c.attrs;
                it->s.edits = c.edits;
                it->s.changes = c.changes;
                it->s.completion = c.extraLetters > 0;
            }
        }
    }
    // Two words typed as one: the space left out (thisis) or typed as a key
    // above the spacebar (thisbis). Both words of one language, each as
    // typed; never for a word of a typing language (compounds stay whole).
    if (!wordInSomeLanguage && (std::isfinite(m_params.missingSpaceCost) || std::isfinite(m_params.spaceKeyCost))) {
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
                    pair = std::exp((static_cast<int>(nw.second) - 255) * format::LogStep);
                    break;
                }
            return std::log(m_params.pairWeight * pair + (1.0 - m_params.pairWeight) * std::exp(uni));
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
                const double cost = tapCost + extra - std::log(w[i]) + capitalCost(i, ls)
                        - m_params.corrector.lmWeight * (logProb(d, ln, ls, prev) + logProb(d, rn, rs, ln));
                const std::u32string word = ls + U' ' + rs;
                const std::u32string key = lower(word);
                auto it = std::find_if(merged.begin(), merged.end(), [&](const Merged &m) { return m.key == key; });
                if (it == merged.end()) {
                    merged.push_back({});
                    it = merged.end() - 1;
                    it->key = key;
                }
                it->prob += std::exp(-cost);
                it->probEqual += std::exp(-cost) / (w[i] * m_dicts.size());
                it->s.languages |= 1u << i;
                if (cost < it->bestCost) {
                    it->bestCost = cost;
                    it->s.word = word;
                    it->s.attrs = d.attributes(ln) | d.attributes(rn);
                    it->s.edits = 1;
                    it->s.changes = 1;
                    it->s.completion = false;
                }
            }
        };
        for (size_t k = minLen; k + minLen <= n; ++k) {
            if (std::isfinite(m_params.missingSpaceCost))
                trySplit(k, k, m_params.missingSpaceCost);
            if (std::isfinite(m_params.spaceKeyCost) && k + 1 + minLen <= n && aboveSpace(taps[k].ch))
                trySplit(k, k + 1, m_params.spaceKeyCost);
        }
    }
    return out;
}

Result Engine::decide(Searched in, const EngineParams &params) const
{
    Result r;
    r.typed = in.typed;
    if (!in.searched)
        return r;
    // The search's findings, under the names process() gave them; params
    // as m_params (the decision's settings may differ from the search's:
    // isDecisionParam).
    const EngineParams &p = params;
    const std::vector<Tap> &taps = in.taps;
    const std::vector<double> &w = in.weights;
    const std::u32string typedLower = lower(r.typed);
    r.typedIsWord = in.typedIsWord;
    std::u32string caseFix = in.caseFix;
    const bool wordInSomeLanguage = in.wordInSomeLanguage, validAsTyped = in.validAsTyped;
    const bool typedHasShortcut = in.typedHasShortcut, typedHasSlipShortcut = in.typedHasSlipShortcut;
    using Merged = Searched::Merged;
    std::vector<Merged> &merged = in.merged;

    // A digit in the word (typed on the number row: h3llo) can only be a
    // slip onto the digit: the fix is the word as typed with a letter in the
    // digit's place. Other words are only offered (any other fix turns names
    // into words: ps4 → is, h2o → who). A letter and a digit is a code
    // (A4, F1, i5): never fixed.
    auto isDigit = [](char32_t c) { return c >= U'0' && c <= U'9'; };
    const bool digits = std::any_of(r.typed.begin(), r.typed.end(), isDigit);
    const std::u32string typedFolded = fold(r.typed);
    auto canFix = [&](const Suggestion &s) {
        if (s.completion)
            return false;
        if (!digits)
            return true;
        const std::u32string b = fold(s.word);
        if (b.size() != typedFolded.size() || b.size() < 3)
            return false;
        for (size_t i = 0; i < b.size(); ++i)
            if (b[i] != typedFolded[i] && !isDigit(typedFolded[i]))
                return false;
        return true;
    };

    // Letters that are a word only in another case (jaime: the name Jaime,
    // mn: MN) are the user's word with a capital only if that is the
    // likeliest reading; else (j'aime, m'n) the candidates compete as for
    // any other letters.
    if (r.typedIsWord && !validAsTyped) {
        const Merged *likeliest = nullptr;
        for (const Merged &m : merged)
            if (canFix(m.s) && (!likeliest || m.prob > likeliest->prob))
                likeliest = &m;
        if (likeliest && likeliest->key != typedLower) {
            r.typedIsWord = false;
            caseFix.clear();
        }
    }

    // The "intended word missing from the dictionaries" hypothesis: its
    // letters cost by how much they look like a word of the likely
    // languages (Kaito less than xqzt), mixed by the languages' weights as
    // the words are. Not when they are a word's but for accents,
    // apostrophes or spelling rules (garcon, perche, strasse): the letter
    // model knows those letters from that word; nor when they are a word of
    // a less likely language (thier, a German surname, in English text): not
    // a missing word, otherLanguageMargin decides. probEqual: all languages
    // equally likely (the first word).
    double unknownProb = 0, unknownProbEqual = 0;
    if (!r.typedIsWord) {
        double tapCost = 0;
        for (const Tap &t : taps) {
            const int k = m_keys.indexOf(t.ch);
            tapCost += k >= 0 ? -std::log(m_keys.probability(k, k)) : 0.0;
        }
        const double base = p.unknownBase;
        const bool spelling = std::any_of(merged.begin(), merged.end(), [&](const Merged &m) {
            return canFix(m.s) && m.s.changes == 0;
        });
        const bool useLetters = p.unknownLetterWeight > 0 && !spelling && !wordInSomeLanguage;
        // Long letters one change from a word are mostly that word
        // misspelled (recieve, goverment), short ones are often new words
        // (Ugo, Norn): when the likeliest fix is one word one change away,
        // what the letter model takes off the cost per letter shrinks with
        // each letter past unknownShortLength.
        const Merged *likeliest = nullptr;
        for (const Merged &m : merged)
            if (canFix(m.s) && (!likeliest || m.prob > likeliest->prob))
                likeliest = &m;
        const bool nearWord = likeliest && (likeliest->s.changes <= 1
                                            || (likeliest->s.changes == 2 && taps.size() >= p.unknownTwoChangeLength))
                              && likeliest->key.find(U' ') == std::u32string::npos;
        const double flat = p.unknownPerLetter * taps.size();
        const double longer = nearWord && taps.size() > p.unknownShortLength
                              ? p.unknownLongLetterCost * (taps.size() - p.unknownShortLength)
                              : 0.0;
        double mixed = 0, equal = 0;
        for (size_t i = 0; i < m_dicts.size(); ++i) {
            const double letters = useLetters ? m_dicts[i]->letterCost(typedFolded) : -1.0;
            double cost = flat;
            if (letters >= 0) {
                cost = p.unknownLetterWeight * (letters - p.unknownLetterRef);
                if (cost < flat)
                    cost = std::min(flat, cost + longer);
            }
            const double p = std::exp(-cost);
            mixed += w[i] * p;
            equal += p / m_dicts.size();
        }
        unknownProb = std::exp(-(tapCost + base)) * mixed;
        unknownProbEqual = std::exp(-(tapCost + base)) * equal;
    }

    // Completions don't compete with corrections for confidence: they are
    // words the user may still be typing.
    double total = unknownProb;
    for (Merged &m : merged) {
        m.s.cost = -std::log(m.prob);
        if (canFix(m.s))
            total += m.prob;
    }
    r.unknownShare = total > 0 ? unknownProb / total : 0;
    std::sort(merged.begin(), merged.end(),
              [](const Merged &a, const Merged &b) { return a.prob > b.prob; });

    for (Merged &m : merged) {
        if ((m.s.attrs & format::AttrOffensive) && lower(m.s.word) != typedLower)
            continue;   // never offered unprompted
        m.s.word = applyCase(m.s.word, taps);
        if (m.s.word == r.typed)
            continue;
        r.suggestions.push_back(m.s);
    }
    if (!caseFix.empty()) {
        auto it = std::find_if(r.suggestions.begin(), r.suggestions.end(),
                               [&](const Suggestion &s) { return s.word == caseFix; });
        if (it != r.suggestions.end())
            std::rotate(r.suggestions.begin(), it, it + 1);
    }
    if (r.suggestions.empty()) {
        r.reason = digits ? Reason::Digits : Reason::NoCandidate;
        return r;
    }

    // Autocorrect considers complete words only: at Space the word is
    // finished, so the candidate is the best suggestion of the typed length.
    int candidate = -1;
    for (size_t i = 0; i < r.suggestions.size(); ++i) {
        if (canFix(r.suggestions[i])) {
            candidate = static_cast<int>(i);
            break;
        }
    }
    if (candidate < 0) {
        r.reason = digits ? Reason::Digits : Reason::NoCandidate;
        return r;
    }
    const Suggestion best = r.suggestions[candidate];

    // Its share of the probability of all complete words and of "a word
    // missing from the dictionaries".
    const std::u32string bestLower = lower(best.word);
    double bestProb = 0;
    for (const Merged &m : merged)
        if (m.key == bestLower)
            bestProb += m.prob;
    r.confidence = total > 0 ? bestProb / total : 0;

    bool languageOk = false;
    double bestWeight = 0;
    for (size_t i = 0; i < m_dicts.size(); ++i)
        if (best.languages & (1u << i))
            bestWeight = std::max(bestWeight, w[i]);
    languageOk = bestWeight >= p.autocorrectConfidence;
    if (!languageOk && in.textWords == 0 && bestWeight >= p.firstWordConfidence) {
        // The first word: the same correction whichever language the text
        // is in, and sure enough with the lean and without it.
        double totalEqual = unknownProbEqual, bestEqual = 0, topEqual = 0;
        for (const Merged &m : merged) {
            if (m.s.completion)
                continue;
            totalEqual += m.probEqual;
            topEqual = std::max(topEqual, m.probEqual);
            if (m.key == bestLower)
                bestEqual = m.probEqual;
        }
        const double shareEqual = totalEqual > 0 ? bestEqual / totalEqual : 0;
        languageOk = bestEqual >= topEqual
                     && std::min(r.confidence, shareEqual) >= p.firstWordShare;
    }
    if (!languageOk || (best.attrs & format::AttrOffensive) || p.strength == Strength::Off) {
        r.reason = !languageOk ? Reason::LanguageNotSure : (best.attrs & format::AttrOffensive) ? Reason::Offensive
                                                                                                : Reason::StrengthOff;
        return r;
    }

    // A word of a typing language that seems unlikely in the text (the
    // first words after a switch: Italian "Mi" after English text) is
    // replaced by a different word only if the fix is otherLanguageMargin
    // nats likelier than the word as typed, read as a word of its language
    // with that language's weight. Accents and apostrophes may always be
    // fixed (fur → für in German text, English jai → j'ai in French text).
    auto letters = [this](const std::u32string &w) {
        std::u32string out = fold(w);
        out.erase(std::remove(out.begin(), out.end(), U'\''), out.end());
        return out;
    };
    if (wordInSomeLanguage && !r.typedIsWord && letters(best.word) != letters(r.typed)) {
        if (!std::isfinite(p.otherLanguageMargin)) {
            r.reason = Reason::OtherLanguage;
            return r;
        }
        double typedProb = 0;
        for (size_t i = 0; i < m_dicts.size(); ++i) {
            std::u32string stored;
            const uint32_t node = m_dicts[i]->find(r.typed, &stored);
            if (!node || (m_dicts[i]->attributes(node) & format::AttrMisspelling) || w[i] <= 0)
                continue;
            // A name typed in lower case (suess: Suess) is not that word.
            if (stored != r.typed && lower(stored) == typedLower && applyCase(stored, taps) != r.typed)
                continue;
            const double tapCost = m_correctors[i]->matchCost(taps, stored);
            if (std::isfinite(tapCost))
                typedProb += std::exp(-(tapCost + p.corrector.lmWeight * -m_dicts[i]->logProb(
                                            m_dicts[i]->node(node).freq) - std::log(w[i])));
        }
        if (typedProb > 0 && std::log(bestProb / typedProb) < p.otherLanguageMargin) {
            r.reason = Reason::OtherLanguage;
            return r;
        }
    }

    // A single letter is never replaced, except by itself in another case:
    // English i, stored as a misspelling of I (the only one in the
    // dictionaries). The handler applies it at Space only (i.e. stays).
    const bool caseOnly = taps.size() == 1 && lower(best.word) == typedLower;
    bool apply = false;
    if (r.typedIsWord) {
        // Only a capitalisation fix (haus → Haus) replaces a valid word, and
        // with Strong a much likelier word one slip away (after a word), but
        // not a word that also stands for another (id: I'd, ill: I'll).
        apply = !caseFix.empty() && best.word == caseFix;
        r.reason = apply ? Reason::CaseFix : Reason::TypedIsWord;
        if (!apply && p.strength == Strength::Strong && !typedHasShortcut && !in.prev.empty()
                && !in.sentenceStart
                && taps.size() >= p.strongMinLength
                && letters(best.word) != letters(r.typed)
                && (neighbourSwap(r.typed, best.word) || swapped(r.typed, best.word))) {
            double typedProb = 0;
            for (const Merged &m : merged)
                if (m.key == typedLower)
                    typedProb += m.prob;
            // A word that is mostly a slip: only with the previous word's
            // support (see typedHasSlipShortcut).
            bool supported = !typedHasSlipShortcut;
            for (size_t i = 0; i < m_dicts.size() && !supported; ++i) {
                if (!(best.languages & (1u << i)))
                    continue;
                const uint32_t prevNode = m_dicts[i]->find(in.prev);
                if (!prevNode)
                    continue;
                for (const auto &n : m_dicts[i]->nextWords(prevNode))
                    if (lower(n.first) == bestLower)
                        supported = true;
            }
            r.reason = supported ? Reason::StrongMargin : Reason::SlipShortcut;
            if (supported && typedProb > 0 && bestProb > 0) {
                r.margin = std::log(bestProb / typedProb);
                apply = r.realWordFix = r.margin >= p.strongMargin;
                if (apply)
                    r.reason = Reason::Applied;
            }
        }
    } else if (p.strength == Strength::Gentle) {
        const int maxChanges = taps.size() <= p.gentleShortLength ? 1 : 2;
        apply = r.confidence >= p.gentleConfidence && best.edits <= 1
                && best.changes <= maxChanges && (taps.size() >= p.gentleMinLength || caseOnly);
        // Two letters: a neighbouring key, or only an accent (ca → ça on a
        // layout without ç), with a higher confidence.
        if (!apply && taps.size() == 2 && r.confidence >= p.gentleTwoLetterConfidence)
            apply = neighbourSwap(r.typed, best.word)
                    || (p.gentleTwoLetterAccents && fold(best.word) == fold(r.typed));
        r.reason = apply ? Reason::Applied
                   : r.confidence < p.gentleConfidence ? Reason::BelowGentle
                   : taps.size() < p.gentleMinLength && !caseOnly ? Reason::TooShort
                                                                         : Reason::GentleChanges;
    } else {
        // Shorter words need more, unless only accents or apostrophes are
        // added (ca → ça, jai → j'ai).
        const bool sameLetters = letters(best.word) == letters(r.typed);
        const double threshold = sameLetters || taps.size() > 4 ? p.normalConfidence
                                 : taps.size() <= 2 ? p.normalTwoLetterConfidence
                                                    : p.normalShortConfidence;
        apply = r.confidence >= threshold && (taps.size() >= p.normalMinLength || caseOnly);
        r.reason = apply ? Reason::Applied
                   : !(taps.size() >= p.normalMinLength || caseOnly) ? Reason::TooShort
                   : sameLetters || taps.size() > 4 ? Reason::BelowNormal
                   : taps.size() <= 2 ? Reason::BelowTwoLetter
                                      : Reason::BelowShort;
    }
    if (apply) {
        // What Space will type comes first in the bar.
        std::rotate(r.suggestions.begin(), r.suggestions.begin() + candidate,
                    r.suggestions.begin() + candidate + 1);
        r.autocorrect = 0;
    }
    return r;
}

bool Engine::swapped(const std::u32string &typed, const std::u32string &word) const
{
    const std::u32string a = fold(typed), b = fold(word);
    if (a.size() != b.size())
        return false;
    size_t i = 0;
    while (i < a.size() && a[i] == b[i])
        ++i;
    return i + 1 < a.size() && a[i] == b[i + 1] && a[i + 1] == b[i]
           && a.compare(i + 2, std::u32string::npos, b, i + 2, std::u32string::npos) == 0;
}

bool Engine::neighbourSwap(const std::u32string &typed, const std::u32string &word) const
{
    const std::u32string a = fold(typed), b = fold(word);
    if (a.size() != b.size())
        return false;
    int k = -1;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == b[i])
            continue;
        if (k >= 0)
            return false;
        k = static_cast<int>(i);
    }
    const int tapped = k < 0 ? -1 : m_keys.indexOf(a[k]);
    const int meant = k < 0 ? -1 : m_keys.indexOf(b[k]);
    if (tapped < 0 || meant < 0)
        return false;
    const std::vector<int> near = m_keys.neighbours(tapped);
    return std::find(near.begin(), near.end(), meant) != near.end();
}

std::vector<Suggestion> Engine::predict(const std::u32string &textBefore, size_t max) const
{
    const Context ctx = parseContext(textBefore);
    if (ctx.insideWord)
        return {};   // the cursor is right after a word
    const bool sentenceStart = ctx.sentenceStart;
    const std::u32string &prev = ctx.prev;

    const std::vector<double> &w = m_tracker.weights();
    struct Scored { std::u32string word; std::u32string key; double score; double best; };
    std::vector<Scored> merged;
    auto add = [&](const std::u32string &word, double score) {
        const std::u32string key = lower(word);
        for (Scored &s : merged) {
            if (s.key == key) {
                s.score += score;
                if (score > s.best) {
                    s.best = score;
                    s.word = word;
                }
                return;
            }
        }
        merged.push_back({word, key, score, score});
    };
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        if (w[i] <= 1e-6)
            continue;
        const Dictionary &d = *m_dicts[i];
        std::vector<std::pair<std::u32string, uint8_t>> next;
        double weight = w[i];
        if (sentenceStart) {
            next = d.nextWords(format::PairStart);
        } else {
            const uint32_t node = d.find(prev);
            if (node && !(d.attributes(node) & format::AttrMisspelling))
                next = d.nextWords(node);
        }
        if (next.empty()) {
            next = d.nextWords(format::PairAny);
            weight *= 0.3;   // a fallback: less sure than a real word pair
        }
        for (const auto &n : next)
            add(n.first, weight * std::exp((static_cast<int>(n.second) - 255) * format::LogStep));
    }
    std::sort(merged.begin(), merged.end(),
              [](const Scored &a, const Scored &b) { return a.score > b.score; });

    std::vector<Suggestion> out;
    for (const Scored &s : merged) {
        if (out.size() >= max)
            break;
        Suggestion sug;
        sug.word = s.word;
        if (sentenceStart && !sug.word.empty())
            sug.word[0] = utf8::toUpper(sug.word[0]);
        sug.cost = -std::log(s.score);
        if (std::none_of(out.begin(), out.end(), [&](const Suggestion &o) { return o.word == sug.word; }))
            out.push_back(sug);
    }
    return out;
}

char32_t Engine::resolveTap(const std::u32string &word, char32_t lifted, char32_t initial) const
{
    if (initial == 0 || initial == lifted)
        return lifted;
    auto score = [&](char32_t c) {
        const std::u32string prefix = word + c;
        double best = -1e300;
        const std::vector<double> &w = m_tracker.weights();
        for (size_t i = 0; i < m_dicts.size(); ++i) {
            const int f = m_dicts[i]->prefixBest(prefix);
            if (f >= 0 && w[i] > 0)
                best = std::max(best, std::log(w[i]) + m_dicts[i]->logProb(static_cast<uint8_t>(f)));
        }
        return best;
    };
    return score(initial) > score(lifted) + m_params.tapSwitchMargin ? initial : lifted;
}

const char *reasonName(Reason reason)
{
    switch (reason) {
    case Reason::None: return "none";
    case Reason::Applied: return "applied";
    case Reason::CaseFix: return "case-fix";
    case Reason::NoCandidate: return "no-candidate";
    case Reason::Digits: return "digits";
    case Reason::LanguageNotSure: return "language-not-sure";
    case Reason::Offensive: return "offensive";
    case Reason::StrengthOff: return "strength-off";
    case Reason::OtherLanguage: return "other-language";
    case Reason::TypedIsWord: return "typed-is-word";
    case Reason::StrongMargin: return "strong-margin";
    case Reason::SlipShortcut: return "slip-shortcut";
    case Reason::BelowNormal: return "below-normal";
    case Reason::BelowTwoLetter: return "below-two-letter";
    case Reason::BelowShort: return "below-short";
    case Reason::TooShort: return "too-short";
    case Reason::BelowGentle: return "below-gentle";
    case Reason::GentleChanges: return "gentle-changes";
    }
    return "?";
}

std::vector<std::u32string> words(const std::u32string &text)
{
    std::vector<std::u32string> out;
    std::u32string cur;
    for (size_t i = 0; i <= text.size(); ++i) {
        char32_t c = i < text.size() ? text[i] : U' ';
        if (c == 0x2019 || c == 0x02BC)
            c = U'\'';
        const bool inner = (c == U'\'' || c == U'-') && !cur.empty()
                           && i + 1 < text.size() && isWordChar(text[i + 1]);
        if (isWordChar(c) || inner) {
            cur.push_back(c);
        } else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    return out;
}

std::u32string applyCase(const std::u32string &word, const std::vector<Tap> &taps)
{
    if (taps.empty() || word.empty())
        return word;
    std::u32string out = word;
    const bool allUpper = taps.size() >= 2 && std::all_of(taps.begin(), taps.end(),
                                                          [](const Tap &t) { return t.upper; });
    if (allUpper) {
        for (char32_t &c : out)
            c = utf8::toUpper(c);
    } else if (taps[0].upper) {
        out[0] = utf8::toUpper(out[0]);
    }
    return out;
}

bool isDecisionParam(const std::string &name)
{
    static const char *const names[] = {
        "autocorrectConfidence", "firstWordConfidence", "firstWordShare", "gentleConfidence", "normalConfidence",
        "normalTwoLetterConfidence", "normalShortConfidence", "normalMinLength", "gentleMinLength",
        "gentleShortLength", "gentleTwoLetterConfidence", "gentleTwoLetterAccents", "strongMargin",
        "strongMinLength", "otherLanguageMargin", "unknownBase", "unknownPerLetter", "unknownLetterWeight",
        "unknownLetterRef", "unknownShortLength", "unknownLongLetterCost", "unknownTwoChangeLength", "strength"};
    return std::find(std::begin(names), std::end(names), name) != std::end(names);
}

bool setParam(EngineParams &p, const std::string &assignment)
{
    if (setParam(p.corrector, assignment) || setParam(p.language, assignment))
        return true;
    const size_t eq = assignment.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string name = assignment.substr(0, eq);
    const double v = std::stod(assignment.substr(eq + 1));
    if (name == "validThreshold") p.validThreshold = v;
    else if (name == "autocorrectConfidence") p.autocorrectConfidence = v;
    else if (name == "firstWordConfidence") p.firstWordConfidence = v;
    else if (name == "firstWordShare") p.firstWordShare = v;
    else if (name == "gentleTwoLetterConfidence") p.gentleTwoLetterConfidence = v;
    else if (name == "gentleTwoLetterAccents") p.gentleTwoLetterAccents = v != 0;
    else if (name == "gentleConfidence") p.gentleConfidence = v;
    else if (name == "normalConfidence") p.normalConfidence = v;
    else if (name == "normalTwoLetterConfidence") p.normalTwoLetterConfidence = v;
    else if (name == "normalShortConfidence") p.normalShortConfidence = v;
    else if (name == "normalMinLength") p.normalMinLength = static_cast<size_t>(v);
    else if (name == "strongMargin") p.strongMargin = v;
    else if (name == "otherLanguageMargin") p.otherLanguageMargin = v;
    else if (name == "missingSpaceCost") p.missingSpaceCost = v;
    else if (name == "spaceKeyCost") p.spaceKeyCost = v;
    else if (name == "splitMinLength") p.splitMinLength = static_cast<size_t>(v);
    else if (name == "strongMinLength") p.strongMinLength = static_cast<size_t>(v);

    else if (name == "gentleMinLength") p.gentleMinLength = static_cast<size_t>(v);
    else if (name == "tapSwitchMargin") p.tapSwitchMargin = v;
    else if (name == "pairWeight") p.pairWeight = v;
    else if (name == "gentleShortLength") p.gentleShortLength = static_cast<size_t>(v);
    else if (name == "completions") p.corrector.completions = v != 0;
    else if (name == "exactMisspellingBonus") p.corrector.exactMisspellingBonus = static_cast<float>(v);
    else if (name == "unknownBase") p.unknownBase = v;
    else if (name == "unknownPerLetter") p.unknownPerLetter = v;
    else if (name == "unknownLetterWeight") p.unknownLetterWeight = v;
    else if (name == "unknownLetterRef") p.unknownLetterRef = v;
    else if (name == "unknownShortLength") p.unknownShortLength = static_cast<size_t>(v);
    else if (name == "unknownTwoChangeLength") p.unknownTwoChangeLength = static_cast<size_t>(v);
    else if (name == "unknownLongLetterCost") p.unknownLongLetterCost = v;
    else if (name == "capitalLowerCost") p.capitalLowerCost = v;
    else if (name == "capitalLowerCostNouns") p.capitalLowerCostNouns = v;
    else if (name == "keyHeightShare") p.keyHeightShare = v;
    else if (name == "searchMargin") p.searchMargin = v;
    else if (name == "searchMinWeight") p.searchMinWeight = v;
    else if (name == "strength") p.strength = v <= 0 ? Strength::Off : v < 2 ? Strength::Gentle
                                       : v < 3 ? Strength::Normal : Strength::Strong;
    else return false;
    return true;
}

} // namespace parrotfish
