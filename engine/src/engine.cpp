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
    m_keys = keys;
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

    // Is the typed word valid in a likely language? In any typing language?
    const std::u32string typedLower = lower(r.typed);
    std::u32string caseFix;   // the stored capitalisation, if it differs (haus → Haus)
    bool wordInSomeLanguage = false;
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        std::u32string stored;
        const uint32_t node = m_dicts[i]->find(r.typed, &stored);
        if (!node || (m_dicts[i]->attributes(node) & format::AttrMisspelling))
            continue;
        wordInSomeLanguage = true;
        if (w[i] < m_params.validThreshold)
            continue;
        r.typedIsWord = true;
        if (stored != r.typed && lower(stored) == typedLower && caseFix.empty()
                && applyCase(stored, taps) != r.typed)
            caseFix = applyCase(stored, taps);
    }

    // Search every language, weighted by its probability, and merge.
    // Spellings that differ only in case (zweiten, Zweiten) are one
    // suggestion, shown in its most likely spelling.
    struct Merged { Suggestion s; std::u32string key; double prob = 0; double bestCost = 1e300; };
    std::vector<Merged> merged;
    const bool useContext = m_params.pairWeight > 0 && !m_context.insideWord;
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        if (w[i] <= 1e-6)
            continue;
        const Dictionary &d = *m_dicts[i];
        std::vector<Candidate> cands = m_correctors[i]->search(taps, std::log(w[i]));

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
            // Likely next words the search didn't reach (it ranks by P(word)).
            for (const auto &n : next) {
                const std::u32string key = lower(n.first);
                if (std::any_of(cands.begin(), cands.end(),
                                [&](const Candidate &c) { return lower(c.word) == key; }))
                    continue;
                int edits = 0, changes = 0;
                const double tapCost = m_correctors[i]->matchCost(taps, n.first, &edits, &changes);
                if (!std::isfinite(tapCost))
                    continue;
                Candidate c;
                c.word = n.first;
                const uint32_t node = d.findExact(n.first);
                c.freq = node ? d.node(node).freq : 0;
                c.attrs = node ? d.attributes(node) : 0;
                c.edits = edits;
                c.changes = changes;
                c.tapCost = tapCost;
                c.wordCost = -(m_params.corrector.lmWeight * d.logProb(c.freq) + std::log(w[i]));
                c.cost = tapCost + c.wordCost;
                rescore(c);
                cands.push_back(c);
            }
        }

        for (const Candidate &c : cands) {
            const std::u32string key = lower(c.word);
            auto it = std::find_if(merged.begin(), merged.end(),
                                   [&](const Merged &m) { return m.key == key; });
            if (it == merged.end()) {
                merged.push_back({});
                it = merged.end() - 1;
                it->key = key;
            }
            it->prob += std::exp(-c.cost);
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
    // The "intended word missing from the dictionaries" hypothesis.
    double unknownProb = 0;
    if (!r.typedIsWord) {
        double tapCost = 0;
        for (const Tap &t : taps) {
            const int k = m_keys.indexOf(t.ch);
            tapCost += k >= 0 ? -std::log(m_keys.probability(k, k)) : 0.0;
        }
        const double base = m_params.strength == Strength::Normal ? m_params.normalUnknownBase
                                                                   : m_params.unknownBase;
        unknownProb = std::exp(-(tapCost + base + m_params.unknownPerLetter * taps.size()));
    }

    // Completions don't compete with corrections for confidence: they are
    // words the user may still be typing.
    double total = unknownProb;
    for (Merged &m : merged) {
        m.s.cost = -std::log(m.prob);
        if (!m.s.completion)
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

    // Autocorrect considers complete words only: at Space the word is
    // finished, so the candidate is the best suggestion of the typed length.
    int candidate = -1;
    for (size_t i = 0; i < r.suggestions.size(); ++i) {
        if (!r.suggestions[i].completion) {
            candidate = static_cast<int>(i);
            break;
        }
    }
    if (candidate < 0)
        return r;
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
    for (size_t i = 0; i < m_dicts.size(); ++i)
        if ((best.languages & (1u << i)) && w[i] >= m_params.autocorrectConfidence)
            languageOk = true;
    if (!languageOk || (best.attrs & format::AttrOffensive) || m_params.strength == Strength::Off)
        return r;

    // A word of any typing language is never replaced by a different word,
    // even when its language seems unlikely (the first words after a switch:
    // Italian "Mi" after English text). Accents may still be fixed (fur → für
    // in German text).
    if (wordInSomeLanguage && !r.typedIsWord && fold(best.word) != fold(r.typed))
        return r;

    bool apply = false;
    if (r.typedIsWord) {
        // Only a capitalisation fix (haus → Haus) replaces a valid word.
        apply = !caseFix.empty() && best.word == caseFix;
    } else if (m_params.strength == Strength::Gentle) {
        const int maxChanges = taps.size() <= m_params.gentleShortLength ? 1 : 2;
        apply = r.confidence >= m_params.gentleConfidence && best.edits <= 1
                && best.changes <= maxChanges && taps.size() >= m_params.gentleMinLength;
    } else {
        apply = r.confidence >= m_params.normalConfidence;
    }
    if (apply) {
        // What Space will type comes first in the bar.
        std::rotate(r.suggestions.begin(), r.suggestions.begin() + candidate,
                    r.suggestions.begin() + candidate + 1);
        r.autocorrect = 0;
    }
    return r;
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
    else if (name == "tapSwitchMargin") p.tapSwitchMargin = v;
    else if (name == "pairWeight") p.pairWeight = v;
    else if (name == "gentleShortLength") p.gentleShortLength = static_cast<size_t>(v);
    else if (name == "completions") p.corrector.completions = v != 0;
    else if (name == "unknownBase") p.unknownBase = v;
    else if (name == "unknownPerLetter") p.unknownPerLetter = v;
    else if (name == "normalUnknownBase") p.normalUnknownBase = v;
    else if (name == "strength") p.strength = v <= 0 ? Strength::Off : v < 2 ? Strength::Gentle : Strength::Normal;
    else return false;
    return true;
}

} // namespace parrotfish
