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

void Engine::setLayout(const KeyModel &keys)
{
    m_keys = keys;
    rebuild();
}

void Engine::rebuild()
{
    m_correctors.clear();
    std::vector<const Dictionary *> dicts;
    for (const auto &d : m_dicts) {
        m_correctors.push_back(std::make_unique<Corrector>(*d, m_keys, m_params.corrector));
        dicts.push_back(d.get());
    }
    m_tracker = LanguageTracker(dicts, m_params.language);
}

void Engine::startText(const std::u32string &textBefore, const std::vector<double> &prior)
{
    m_tracker.reset(prior);
    for (const std::u32string &w : words(textBefore))
        m_tracker.observe(w);
}

void Engine::commit(const std::u32string &word)
{
    m_tracker.observe(word);
}

Result Engine::process(const std::vector<Tap> &taps) const
{
    Result r;
    for (const Tap &t : taps)
        r.typed.push_back(t.upper ? utf8::toUpper(t.ch) : t.ch);
    if (taps.empty() || m_dicts.empty() || taps.size() > MaxTaps)
        return r;
    const std::vector<double> &w = m_tracker.weights();

    // Is the typed word valid in a likely language?
    const std::u32string typedLower = lower(r.typed);
    std::u32string caseFix;   // the stored capitalisation, if it differs (haus → Haus)
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        if (w[i] < m_params.validThreshold)
            continue;
        std::u32string stored;
        const uint32_t node = m_dicts[i]->find(r.typed, &stored);
        if (!node || (m_dicts[i]->attributes(node) & format::AttrMisspelling))
            continue;
        r.typedIsWord = true;
        if (stored != r.typed && lower(stored) == typedLower && caseFix.empty()
                && applyCase(stored, taps) != r.typed)
            caseFix = applyCase(stored, taps);
    }

    // Search every language, weighted by its probability, and merge.
    struct Merged { Suggestion s; double prob = 0; double bestCost = 1e300; };
    std::vector<Merged> merged;
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        if (w[i] <= 1e-6)
            continue;
        for (const Candidate &c : m_correctors[i]->search(taps, std::log(w[i]))) {
            auto it = std::find_if(merged.begin(), merged.end(),
                                   [&](const Merged &m) { return m.s.word == c.word; });
            if (it == merged.end()) {
                merged.push_back({});
                it = merged.end() - 1;
                it->s.word = c.word;
            }
            it->prob += std::exp(-c.cost);
            it->s.languages |= 1u << i;
            if (c.cost < it->bestCost) {
                it->bestCost = c.cost;
                it->s.attrs = c.attrs;
                it->s.edits = c.edits;
            }
        }
    }
    // The "intended word missing from the dictionaries" hypothesis.
    double unknownProb = 0;
    if (!r.typedIsWord) {
        double tapCost = 0;
        for (const Tap &t : taps) {
            const int k = m_keys.indexOf(t.ch);
            tapCost += k >= 0 ? -std::log(m_keys.probability(k, k)) : 0.0;
        }
        unknownProb = std::exp(-(tapCost + m_params.unknownBase
                                 + m_params.unknownPerLetter * taps.size()));
    }

    double total = unknownProb;
    for (Merged &m : merged) {
        m.s.cost = -std::log(m.prob);
        total += m.prob;
    }
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
    if (r.suggestions.empty())
        return r;

    // Share of probability of the best suggestion (same word in any case form).
    const std::u32string bestLower = lower(r.suggestions[0].word);
    double bestProb = 0;
    for (const Merged &m : merged)
        if (lower(m.s.word) == bestLower)
            bestProb += m.prob;
    r.confidence = total > 0 ? bestProb / total : 0;

    // Autocorrect decision.
    const Suggestion &best = r.suggestions[0];
    bool languageOk = false;
    for (size_t i = 0; i < m_dicts.size(); ++i)
        if ((best.languages & (1u << i)) && w[i] >= m_params.autocorrectConfidence)
            languageOk = true;
    if (!languageOk || (best.attrs & format::AttrOffensive))
        return r;
    if (r.typedIsWord) {
        // Only a capitalisation fix (haus → Haus) replaces a valid word.
        if (!caseFix.empty() && best.word == caseFix && m_params.strength != Strength::Off)
            r.autocorrect = 0;
        return r;
    }
    switch (m_params.strength) {
    case Strength::Off:
        break;
    case Strength::Gentle:
        if (r.confidence >= m_params.gentleConfidence && best.edits <= 1
                && taps.size() >= m_params.gentleMinLength)
            r.autocorrect = 0;
        break;
    case Strength::Normal:
        if (r.confidence >= m_params.normalConfidence)
            r.autocorrect = 0;
        break;
    }
    return r;
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
    else if (name == "gentleConfidence") p.gentleConfidence = v;
    else if (name == "normalConfidence") p.normalConfidence = v;
    else if (name == "gentleMinLength") p.gentleMinLength = static_cast<size_t>(v);
    else if (name == "unknownBase") p.unknownBase = v;
    else if (name == "unknownPerLetter") p.unknownPerLetter = v;
    else if (name == "strength") p.strength = v <= 0 ? Strength::Off : v < 2 ? Strength::Gentle : Strength::Normal;
    else return false;
    return true;
}

} // namespace parrotfish
