// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"
#include "engine-internal.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace parrotfish {

using internal::isWordChar;
using internal::lower;

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
    m_contextNext.clear();
    m_learnedCache.clear();
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
    std::unordered_map<std::u32string, size_t> index;   // key -> merged
    auto add = [&](const std::u32string &word, double score) {
        std::u32string key = lower(word);
        const auto found = index.find(key);
        if (found != index.end()) {
            Scored &s = merged[found->second];
            s.score += score;
            if (score > s.best) {
                s.best = score;
                s.word = word;
            }
            return;
        }
        index.emplace(key, merged.size());
        merged.push_back({word, std::move(key), score, score});
    };
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        if (w[i] <= 1e-6)
            continue;
        for (const auto &n : nextWords(i, sentenceStart, prev, true))
            add(n.first, w[i] * n.second);
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

std::vector<std::pair<std::u32string, double>> Engine::nextWords(size_t i, bool sentenceStart,
                                                                 const std::u32string &prev, bool anyFallback) const
{
    const Dictionary &d = *m_dicts[i];
    std::vector<std::pair<std::u32string, uint8_t>> stored;
    if (sentenceStart) {
        stored = d.nextWords(format::PairStart);
    } else if (!prev.empty()) {
        const uint32_t node = d.find(prev);
        if (node && !(d.attributes(node) & format::AttrMisspelling))
            stored = d.nextWords(node);
    }
    double scale = 1.0;
    if (stored.empty() && anyFallback) {
        stored = d.nextWords(format::PairAny);
        scale = 0.3;   // a fallback: less sure than a real word pair
    }
    std::vector<std::pair<std::u32string, double>> out;
    out.reserve(stored.size());
    for (auto &n : stored)
        out.emplace_back(std::move(n.first),
                         scale * std::exp((static_cast<int>(n.second) - 255) * format::LogStep));

    // The user's next words after the same word.
    const LearnedCache &lc = learnedCache(i);
    if (!lc.lang || (!sentenceStart && prev.empty()))
        return out;
    const std::u32string prevKey = sentenceStart ? std::u32string() : lower(prev);
    const auto next = lc.lang->next.find(prevKey);
    if (next == lc.lang->next.end())
        return out;
    const double m = lc.lang->nextTotal.at(prevKey);
    const double nu = m_params.learnedPairWeight * m / (m + m_params.learnedPairK);
    if (m <= 0 || nu <= 0)
        return out;
    std::unordered_map<std::u32string, size_t> index;
    for (size_t k = 0; k < out.size(); ++k) {
        out[k].second *= 1.0 - nu;
        index.emplace(lower(out[k].first), k);
    }
    // The user's likeliest followers, as many as the dictionary keeps.
    constexpr size_t MaxUserNext = 64;
    std::vector<std::pair<std::u32string, double>> user(next->second.begin(), next->second.end());
    if (user.size() > MaxUserNext) {
        std::nth_element(user.begin(), user.begin() + MaxUserNext - 1, user.end(),
                         [](const std::pair<std::u32string, double> &a, const std::pair<std::u32string, double> &b) {
                             return a.second > b.second;
                         });
        user.resize(MaxUserNext);
    }
    for (const auto &n : user) {
        const double p = nu * n.second / m;
        const auto found = index.find(n.first);
        if (found != index.end()) {
            out[found->second].second += p;
            continue;
        }
        // A word in use (pairs are kept between real words: known or
        // learned), as the user types it.
        const Learned::Word *lw = lc.lang->word(n.first);
        if (!lw)
            continue;
        index.emplace(n.first, out.size());
        out.emplace_back(lw->spelling, p);
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const std::pair<std::u32string, double> &a, const std::pair<std::u32string, double> &b) {
                         return a.second > b.second;
                     });
    return out;
}

bool Engine::isKnown(const std::u32string &word) const
{
    const std::u32string key = lower(word);
    for (const auto &d : m_dicts) {
        const uint32_t node = d->find(word);
        if (node ? !(d->attributes(node) & format::AttrMisspelling) : d->isWordForm(key))
            return true;
    }
    return false;
}

std::u32string Engine::previousWord(const std::u32string &textBefore)
{
    const Context ctx = parseContext(textBefore);
    return ctx.sentenceStart ? std::u32string() : ctx.prev;
}

void Engine::learn(Learned &learned, const std::u32string &textBefore, const std::u32string &word,
                   const std::string &language) const
{
    const int lead = m_tracker.leading();
    if ((lead < 0 && language.empty()) || word.empty())
        return;
    const Context ctx = parseContext(textBefore);
    const std::u32string prev = ctx.sentenceStart ? std::u32string() : ctx.prev;
    learned.add(language.empty() ? this->language(static_cast<size_t>(lead)) : language, word, isKnown(word), prev,
                !prev.empty() && isKnown(prev));
}

double Engine::learnedUni(const LearnedCache &lc, const std::u32string &key, double dictUni)
{
    if (!lc.lang)
        return dictUni;
    const Learned::Word *w = lc.lang->word(key);
    const double user = w ? w->count / lc.lang->total : 0.0;
    return (1.0 - lc.mu) * dictUni + lc.mu * user;
}

const Engine::LearnedCache &Engine::learnedCache(size_t i) const
{
    if (m_learnedCache.size() != m_dicts.size())
        m_learnedCache.assign(m_dicts.size(), LearnedCache());
    LearnedCache &c = m_learnedCache[i];
    const uint64_t version = m_learned ? m_learned->version() : 0;
    if (c.version == version)
        return c;
    c.version = version;
    c.lang = nullptr;
    c.mu = 0;
    const Learned::Language *lang = m_learned && m_params.learnedWeight > 0
                                    ? m_learned->language(m_dicts[i]->language()) : nullptr;
    if (!lang || lang->total <= 0) {
        c.reach.clear();
        c.reachLetters.clear();
        c.reachVersion = c.reachNewWords = UINT64_MAX;
        return c;
    }
    c.lang = lang;
    const double n = lang->total;
    c.mu = m_params.learnedWeight * n / (n + m_params.learnedK);
    // The search also tries the learned new words, and the dictionary words
    // the user makes much likelier (among the most used). Rebuilt when the
    // new words change, or every 256 words typed.
    constexpr uint64_t ReachEvery = 256;
    if (c.reachNewWords == m_learned->newWordsVersion() && c.reachVersion != UINT64_MAX
            && version - c.reachVersion < ReachEvery)
        return c;
    c.reachNewWords = m_learned->newWordsVersion();
    c.reachVersion = version;
    c.reach.clear();
    c.reachLetters.clear();
    const Dictionary &d = *m_dicts[i];
    std::vector<std::pair<double, const std::u32string *>> used;
    for (const auto &e : lang->words) {
        const Learned::Word &w = e.second;
        if (w.pending)
            continue;
        if (w.isNew)
            c.reach.push_back({w.spelling, e.first, true, 0, 0});
        else if (m_params.learnedReach > 0)
            used.emplace_back(w.count, &e.first);
    }
    const size_t top = std::min(used.size(), m_params.learnedReach);
    std::partial_sort(used.begin(), used.begin() + static_cast<long>(top), used.end(),
                      [](const std::pair<double, const std::u32string *> &a,
                         const std::pair<double, const std::u32string *> &b) { return a.first > b.first; });
    for (size_t k = 0; k < top; ++k) {
        std::u32string stored;
        const uint32_t node = d.find(*used[k].second, &stored);
        if (!node)
            continue;
        const uint32_t attrs = d.attributes(node);
        if (attrs & format::AttrMisspelling)
            continue;
        const uint8_t freq = d.node(node).freq;
        const double dictUni = std::exp(d.logProb(freq));
        if (learnedUni(c, *used[k].second, dictUni) >= m_params.learnedReachRatio * dictUni)
            c.reach.push_back({stored, *used[k].second, false, freq, attrs});
    }
    for (const LearnedCache::Reach &r : c.reach)
        c.reachLetters.push_back(m_correctors[i]->letters(r.word));
    return c;
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



} // namespace parrotfish
