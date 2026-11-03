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
