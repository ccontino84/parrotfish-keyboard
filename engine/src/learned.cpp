// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "parrotfish/learned.h"
#include "engine-internal.h"

#include <algorithm>
#include <tuple>

namespace parrotfish {

using internal::lower;

bool learnableNewWord(const std::u32string &word)
{
    if (word.size() < 2)
        return false;
    bool allCapitals = true;
    for (char32_t c : word) {
        if (c >= U'0' && c <= U'9')
            return false;
        if (utf8::toLower(c) == c && utf8::toUpper(c) != c)
            allCapitals = false;
    }
    return !(allCapitals && word.size() > 4);
}

const Learned::Word *Learned::Language::word(const std::u32string &key) const
{
    const auto it = words.find(key);
    return it == words.end() || it->second.pending ? nullptr : &it->second;
}

Learned::Learned(LearnParams params)
    : m_params(params)
{
}

Learned::Language &Learned::lang(const std::string &code)
{
    Language &l = m_languages[code];
    if (l.nextFade <= 0)
        l.nextFade = m_params.fadeEvery;
    return l;
}

const Learned::Language *Learned::language(const std::string &code) const
{
    const auto it = m_languages.find(code);
    return it == m_languages.end() ? nullptr : &it->second;
}

std::vector<std::string> Learned::languages() const
{
    std::vector<std::string> out;
    for (const auto &l : m_languages)
        out.push_back(l.first);
    std::sort(out.begin(), out.end());
    return out;
}

bool Learned::isLearnedNew(const std::u32string &word) const
{
    const std::u32string key = lower(word);
    for (const auto &l : m_languages) {
        const Word *w = l.second.word(key);
        if (w && w->isNew)
            return true;
    }
    return false;
}

void Learned::endText()
{
    ++m_text;
}

void Learned::add(const std::string &language, const std::u32string &word, bool known,
                  const std::u32string &prev, bool prevKnown)
{
    if (word.empty() || std::any_of(word.begin(), word.end(), [](char32_t c) { return c >= U'0' && c <= U'9'; }))
        return;
    if (!known && !learnableNewWord(word))
        return;
    Language &l = lang(language);
    ++m_version;
    l.typed += 1;
    const std::u32string key = lower(word);
    m_changes.languages.insert(language);
    m_changes.words.emplace(language, key);
    Word &w = l.words[key];
    if (known) {
        if (w.pending) {   // a word the dictionaries have now (a language added)
            w.pending = false;
            l.total += w.count;
        }
        if (w.isNew)
            ++m_newWordsVersion;
        w.isNew = false;
        if (w.spelling.empty() || !prev.empty())
            w.spelling = word;
        w.count += 1;
        l.total += 1;
        w.lastUsed = l.typed;
    } else {
        if (w.count == 0 && !w.pending) {   // first kept
            w.isNew = true;
            w.pending = true;
            w.firstSeen = l.typed;
        }
        if (w.pending && l.typed - w.firstSeen > m_params.newWordWindow) {   // too late: start over
            w.count = 0;
            w.texts = 0;
            w.firstSeen = l.typed;
        }
        // Its capitals as typed, but not a sentence start's (Mitton, NASA
        // stay; Hello at a sentence start is hello).
        if (!prev.empty()) {
            w.spelling = word;
        } else if (w.spelling.empty()) {
            w.spelling = word;
            const bool restLower = std::all_of(word.begin() + 1, word.end(),
                                               [](char32_t c) { return utf8::toLower(c) == c; });
            if (restLower)
                w.spelling[0] = utf8::toLower(w.spelling[0]);
        }
        w.count += 1;
        w.lastUsed = l.typed;
        if (w.pending) {
            if (w.lastText != m_text) {
                w.texts += 1;
                w.lastText = m_text;
            }
            if (w.texts >= m_params.newWordTexts) {
                w.pending = false;
                l.total += w.count;
                ++m_newWordsVersion;
            }
        } else {
            l.total += 1;
        }
    }

    // Pairs between real words only, within a sentence.
    const bool real = !w.pending;
    const std::u32string prevKey = lower(prev);
    const bool prevReal = prev.empty() || prevKnown || l.word(prevKey);
    if (real && prevReal) {
        double &c = l.next[prevKey][key];
        if (c == 0)
            ++l.pairs;
        m_changes.pairs.emplace(language, prevKey, key);
        c += 1;
        l.nextTotal[prevKey] += 1;
    }

}

bool Learned::overCaps(const Language &l) const
{
    return l.words.size() > m_params.maxWords + m_params.maxWords / 10
            || l.pairs > m_params.maxPairs + m_params.maxPairs / 10;
}

bool Learned::maintenanceDue() const
{
    for (const auto &entry : m_languages)
        if (entry.second.typed >= entry.second.nextFade || overCaps(entry.second))
            return true;
    return false;
}

void Learned::maintain()
{
    for (auto &entry : m_languages) {
        Language &l = entry.second;
        if (l.typed < l.nextFade && !overCaps(l))
            continue;
        for (; l.typed >= l.nextFade; l.nextFade += m_params.fadeEvery)
            fadeAll(l);
        if (overCaps(l))
            trim(l);
        ++m_version;
        m_changes.rewrite.insert(entry.first);
        m_changes.languages.insert(entry.first);
    }
}

void Learned::fadeAll(Language &l)
{
    ++m_newWordsVersion;
    l.total = 0;
    for (auto it = l.words.begin(); it != l.words.end();) {
        Word &w = it->second;
        if (w.pending) {
            if (l.typed - w.firstSeen > m_params.newWordWindow)
                it = l.words.erase(it);
            else
                ++it;
            continue;
        }
        w.count *= m_params.fade;
        if (w.count < m_params.dropBelow) {
            it = l.words.erase(it);
            continue;
        }
        l.total += w.count;
        ++it;
    }
    l.pairs = 0;
    l.nextTotal.clear();
    for (auto p = l.next.begin(); p != l.next.end();) {
        double sum = 0;
        for (auto it = p->second.begin(); it != p->second.end();) {
            it->second *= m_params.fade;
            if (it->second < m_params.dropBelow) {
                it = p->second.erase(it);
                continue;
            }
            sum += it->second;
            ++it;
        }
        if (p->second.empty()) {
            p = l.next.erase(p);
            continue;
        }
        l.pairs += p->second.size();
        l.nextTotal[p->first] = sum;
        ++p;
    }
}

void Learned::trim(Language &l)
{
    if (l.words.size() > m_params.maxWords) {
        ++m_newWordsVersion;
        // The least used first; of those used as often, the one kept
        // longest ago.
        std::vector<std::tuple<double, double, std::u32string>> byCount;
        byCount.reserve(l.words.size());
        for (const auto &w : l.words)
            byCount.emplace_back(w.second.count, w.second.lastUsed, w.first);
        std::sort(byCount.begin(), byCount.end());
        for (size_t i = 0; i + m_params.maxWords < byCount.size(); ++i)
            l.words.erase(std::get<2>(byCount[i]));
        l.total = 0;
        for (const auto &w : l.words)
            if (!w.second.pending)
                l.total += w.second.count;
    }
    if (l.pairs > m_params.maxPairs) {
        std::vector<std::tuple<double, std::u32string, std::u32string>> byCount;
        byCount.reserve(l.pairs);
        for (const auto &p : l.next)
            for (const auto &n : p.second)
                byCount.emplace_back(n.second, p.first, n.first);
        std::sort(byCount.begin(), byCount.end());
        for (size_t i = 0; i + m_params.maxPairs < byCount.size(); ++i)
            dropPair(l, std::get<1>(byCount[i]), std::get<2>(byCount[i]));
    }
}

void Learned::dropPair(Language &l, const std::u32string &prev, const std::u32string &key)
{
    const auto p = l.next.find(prev);
    if (p == l.next.end())
        return;
    const auto it = p->second.find(key);
    if (it == p->second.end())
        return;
    l.nextTotal[prev] -= it->second;
    p->second.erase(it);
    --l.pairs;
    if (p->second.empty()) {
        l.next.erase(p);
        l.nextTotal.erase(prev);
    }
}

void Learned::forget(const std::u32string &word)
{
    const std::u32string key = lower(word);
    ++m_version;
    ++m_newWordsVersion;
    for (auto &entry : m_languages) {
        Language &l = entry.second;
        m_changes.words.emplace(entry.first, key);
        for (const auto &q : l.next) {
            if (q.first == key)
                for (const auto &n : q.second)
                    m_changes.pairs.emplace(entry.first, key, n.first);
            else if (q.second.count(key))
                m_changes.pairs.emplace(entry.first, q.first, key);
        }
        const auto it = l.words.find(key);
        if (it != l.words.end()) {
            if (!it->second.pending)
                l.total -= it->second.count;
            l.words.erase(it);
        }
        const auto p = l.next.find(key);
        if (p != l.next.end()) {
            l.pairs -= p->second.size();
            l.next.erase(p);
            l.nextTotal.erase(key);
        }
        std::vector<std::u32string> prevs;
        for (const auto &q : l.next)
            if (q.second.count(key))
                prevs.push_back(q.first);
        for (const std::u32string &prev : prevs)
            dropPair(l, prev, key);
    }
}

void Learned::clear()
{
    m_languages.clear();
    m_changes = Changes();
    m_changes.cleared = true;
    ++m_version;
    ++m_newWordsVersion;
}

Learned::Changes Learned::takeChanges()
{
    Changes out;
    std::swap(out, m_changes);
    return out;
}

void Learned::restoreLanguage(const std::string &language, double typed, double nextFade)
{
    Language &l = lang(language);
    l.typed = typed;
    if (nextFade > 0)
        l.nextFade = nextFade;
}

void Learned::restoreWord(const std::string &language, const std::u32string &key, const Word &word)
{
    Word w = word;
    w.lastText = 0;
    lang(language).words[key] = w;
}

void Learned::restorePair(const std::string &language, const std::u32string &prev, const std::u32string &word,
                          double count)
{
    if (count > 0)
        lang(language).next[prev][word] = count;
}

void Learned::finishRestore()
{
    for (auto &entry : m_languages) {
        Language &l = entry.second;
        l.total = 0;
        for (const auto &w : l.words)
            if (!w.second.pending)
                l.total += w.second.count;
        l.pairs = 0;
        l.nextTotal.clear();
        for (const auto &p : l.next) {
            double sum = 0;
            for (const auto &n : p.second)
                sum += n.second;
            l.nextTotal[p.first] = sum;
            l.pairs += p.second.size();
        }
    }
    ++m_version;
    ++m_newWordsVersion;
    m_changes = Changes();
}

bool setParam(LearnParams &p, const std::string &assignment)
{
    const size_t eq = assignment.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string name = assignment.substr(0, eq);
    const double v = std::stod(assignment.substr(eq + 1));
    if (name == "newWordTexts")
        p.newWordTexts = static_cast<int>(v);
    else if (name == "newWordWindow")
        p.newWordWindow = v;
    else if (name == "fadeEvery")
        p.fadeEvery = v;
    else if (name == "fade")
        p.fade = v;
    else if (name == "dropBelow")
        p.dropBelow = v;
    else if (name == "maxWords")
        p.maxWords = static_cast<size_t>(v);
    else if (name == "maxPairs")
        p.maxPairs = static_cast<size_t>(v);
    else
        return false;
    return true;
}

} // namespace parrotfish
