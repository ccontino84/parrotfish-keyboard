// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "parrotfish/corrector.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace parrotfish {

namespace {
constexpr float Inf = std::numeric_limits<float>::infinity();
constexpr uint8_t DefaultTargetFreq = 100;   // multi-word targets ("a lot")
constexpr double ShortcutPenalty = 1.0;
constexpr size_t MaxVisits = 400000;         // latency guard per search
} // namespace

std::vector<Tap> tapsFor(const std::u32string &typed)
{
    std::vector<Tap> taps;
    taps.reserve(typed.size());
    for (char32_t c : typed) {
        Tap t;
        t.ch = utf8::toLower(c);
        t.upper = t.ch != c;
        taps.push_back(t);
    }
    return taps;
}

Corrector::Corrector(const Dictionary &dict, const KeyModel &keys, CorrectorParams params)
    : m_dict(dict)
    , m_keys(keys)
    , m_params(params)
{
    const uint32_t n = dict.alphabetSize();
    m_keyOfLetter.assign(n, -1);
    m_keyOfBase.assign(n, -1);
    for (uint32_t i = 0; i < n; ++i) {
        const format::AlphabetEntry &l = dict.letter(i);
        m_keyOfLetter[i] = keys.indexOf(l.lower);
        m_keyOfBase[i] = keys.indexOf(l.base);
    }
}

struct Corrector::Search
{
    const Corrector &c;
    const Dictionary &d;
    const CorrectorParams &p;
    const std::vector<Tap> &taps;
    double logPrior;
    size_t n;
    std::vector<float> tapCost;      // n × alphabet
    std::vector<double> suffixMin;   // lower bound for taps i..n-1
    std::u32string path;
    std::vector<Candidate> results;
    size_t visits = 0;

    Search(const Corrector &c, const std::vector<Tap> &taps, double logPrior)
        : c(c), d(c.m_dict), p(c.m_params), taps(taps), logPrior(logPrior), n(taps.size())
    {
        const uint32_t a = d.alphabetSize();
        // Base letter of any character the dictionary knows.
        std::unordered_map<char32_t, char32_t> baseOf;
        for (uint32_t i = 0; i < a; ++i)
            baseOf[d.letter(i).lower] = d.letter(i).base;
        auto base = [&](char32_t ch) {
            auto it = baseOf.find(ch);
            return it == baseOf.end() ? ch : it->second;
        };

        tapCost.assign(n * a, Inf);
        for (size_t i = 0; i < n; ++i) {
            const Tap &t = taps[i];
            const int k = c.m_keys.indexOf(t.ch);
            const int k0 = t.initial && t.initial != t.ch ? c.m_keys.indexOf(t.initial) : -1;
            const char32_t tbase = base(t.ch);
            auto spatial = [&](int ki) {
                if (ki < 0)
                    return Inf;
                float prob = c.m_keys.probability(k, ki);
                if (k0 >= 0)
                    prob = p.liftWeight * prob + (1.0f - p.liftWeight) * c.m_keys.probability(k0, ki);
                return prob >= p.minTapProb ? -std::log(prob) : Inf;
            };
            for (uint32_t li = 0; li < a; ++li) {
                const format::AlphabetEntry &l = d.letter(li);
                float cost = Inf;
                if (k >= 0) {
                    // A layout key: the letter's own key, or its plain letter's key
                    // with the accent left out (fur → für, perche → perché).
                    cost = spatial(c.m_keyOfLetter[li]);
                    if (l.base != l.lower)
                        cost = std::min(cost, spatial(c.m_keyOfBase[li]) + p.accentOmitted);
                    if (l.lower != t.ch && l.base == tbase)
                        cost = std::min(cost, p.accentWrong);   // typed ü, meant u
                } else if (l.lower == t.ch) {
                    cost = 0.0f;                 // typed from a popup or the symbols
                } else if (l.base == tbase) {
                    cost = p.accentWrong;
                }
                tapCost[i * a + li] = cost;
            }
        }
        suffixMin.assign(n + 1, 0.0);
        for (size_t i = n; i-- > 0;) {
            float m = p.insertion;
            for (uint32_t li = 0; li < a; ++li)
                m = std::min(m, tapCost[i * a + li]);
            suffixMin[i] = suffixMin[i + 1] + m;
        }
    }

    float tc(size_t i, uint32_t node) const
    {
        return tapCost[i * d.alphabetSize() + d.node(node).ch];
    }

    double wordCost(uint8_t freq) const
    {
        return -(p.lmWeight * d.logProb(freq) + logPrior);
    }

    double worst() const
    {
        return results.size() < p.maxResults ? std::numeric_limits<double>::infinity()
                                              : results.back().cost;
    }

    void add(Candidate cand)
    {
        for (auto it = results.begin(); it != results.end(); ++it) {
            if (it->word == cand.word) {
                if (it->cost <= cand.cost)
                    return;
                results.erase(it);
                break;
            }
        }
        auto pos = std::upper_bound(results.begin(), results.end(), cand.cost,
            [](double v, const Candidate &x) { return v < x.cost; });
        results.insert(pos, std::move(cand));
        if (results.size() > p.maxResults)
            results.pop_back();
    }

    void emit(uint32_t node, double cost, int edits, int extra)
    {
        std::u32string target;
        const uint32_t attrs = d.attributes(node, &target);
        const double wc = wordCost(d.node(node).freq);
        Candidate cand;
        cand.tapCost = cost;
        cand.edits = edits;
        cand.extraLetters = extra;
        cand.attrs = attrs;
        if (attrs & format::AttrMisspelling) {
            if (target.empty())
                return;
            cand.word = target;
            cand.viaTarget = true;
            cand.wordCost = wc;
            cand.cost = cost + wc;
            add(std::move(cand));
            return;
        }
        cand.word = path;
        cand.wordCost = wc;
        cand.cost = cost + wc;
        if ((attrs & format::AttrShortcut) && !target.empty()) {
            Candidate extraCand = cand;
            extraCand.word = target;
            extraCand.viaTarget = true;
            const uint32_t tn = d.findExact(target);
            extraCand.wordCost = wordCost(tn ? d.node(tn).freq : DefaultTargetFreq);
            extraCand.cost = cost + extraCand.wordCost + ShortcutPenalty;
            add(std::move(extraCand));
        }
        add(std::move(cand));
    }

    bool ruleMatches(const format::Rule &r, size_t i, size_t *len) const
    {
        size_t k = 0;
        for (; k < 3 && r.in[k]; ++k) {
            if (i + k >= n || taps[i + k].ch != r.in[k])
                return false;
        }
        if ((r.flags & format::RuleFinal) && i + k != n)
            return false;
        *len = k;
        return k > 0;
    }

    void visit(uint32_t node, size_t i, double cost, int edits, int extra)
    {
        if (++visits > MaxVisits)
            return;
        if (i == n && node != 0 && d.isTerminal(node) && cost + wordCost(d.node(node).freq) < worst())
            emit(node, cost, edits, extra);

        // An extra tap: skip it.
        if (i < n && edits < p.maxEdits)
            visit(node, i + 1, cost + p.insertion, edits + 1, extra);

        uint32_t ch = d.firstChild(node);
        if (ch == 0)
            return;
        for (;; ++ch) {
            const double bound = cost + suffixMin[i] + wordCost(d.node(ch).best);
            if (bound >= worst())
                break;   // siblings are sorted by `best`: the rest is worse
            const char32_t letter = d.letter(d.node(ch).ch).cp;
            path.push_back(letter);

            if (i < n) {
                const float t = tc(i, ch);
                if (t <= p.farTap)
                    visit(ch, i + 1, cost + t, edits, extra);
                else if (t < Inf && edits < p.maxEdits)
                    visit(ch, i + 1, cost + t, edits + 1, extra);
            } else if (p.completions && edits == 0) {
                visit(ch, i, cost + p.completionCost, edits, extra + 1);
            }

            for (uint32_t r = 0; r < d.ruleCount(); ++r) {
                const format::Rule &rule = d.rules()[r];
                size_t len;
                if (rule.out == d.letter(d.node(ch).ch).lower && ruleMatches(rule, i, &len))
                    visit(ch, i + len, cost + p.ruleCost, edits, extra);
            }

            if (letter == U'\'' || letter == U'-' || letter == U'.') {
                visit(ch, i, cost + p.punctuationSkipped, edits, extra);
            } else if (edits < p.maxEdits && (i < n || !p.completions)) {
                // A letter the user left out.
                visit(ch, i, cost + p.deletion, edits + 1, extra);
            }

            // Two taps swapped: the next tap is this letter, this tap the one after.
            if (i + 1 < n && edits < p.maxEdits) {
                const float t1 = tc(i + 1, ch);
                uint32_t g = d.firstChild(ch);
                if (t1 < Inf && g != 0) {
                    for (;; ++g) {
                        const float t0 = tc(i, g);
                        if (t0 < Inf) {
                            path.push_back(d.letter(d.node(g).ch).cp);
                            visit(g, i + 2, cost + p.transposition + t0 + t1, edits + 1, extra);
                            path.pop_back();
                        }
                        if (d.isLastSibling(g))
                            break;
                    }
                }
            }

            path.pop_back();
            if (d.isLastSibling(ch))
                break;
        }
    }
};

bool setParam(CorrectorParams &p, const std::string &assignment)
{
    const size_t eq = assignment.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string name = assignment.substr(0, eq);
    const float v = std::stof(assignment.substr(eq + 1));
    struct { const char *name; float *f; int *i; size_t *z; } table[] = {
        {"liftWeight", &p.liftWeight, nullptr, nullptr},
        {"farTap", &p.farTap, nullptr, nullptr},
        {"minTapProb", &p.minTapProb, nullptr, nullptr},
        {"accentOmitted", &p.accentOmitted, nullptr, nullptr},
        {"accentWrong", &p.accentWrong, nullptr, nullptr},
        {"deletion", &p.deletion, nullptr, nullptr},
        {"insertion", &p.insertion, nullptr, nullptr},
        {"transposition", &p.transposition, nullptr, nullptr},
        {"punctuationSkipped", &p.punctuationSkipped, nullptr, nullptr},
        {"ruleCost", &p.ruleCost, nullptr, nullptr},
        {"lmWeight", &p.lmWeight, nullptr, nullptr},
        {"completionCost", &p.completionCost, nullptr, nullptr},
        {"maxEdits", nullptr, &p.maxEdits, nullptr},
        {"maxResults", nullptr, nullptr, &p.maxResults},
    };
    for (auto &e : table) {
        if (name != e.name)
            continue;
        if (e.f) *e.f = v;
        if (e.i) *e.i = static_cast<int>(v);
        if (e.z) *e.z = static_cast<size_t>(v);
        return true;
    }
    return false;
}

std::vector<Candidate> Corrector::search(const std::vector<Tap> &taps, double logPrior) const
{
    if (taps.empty())
        return {};
    Search s(*this, taps, logPrior);
    s.visit(0, 0, 0.0, 0, 0);
    return std::move(s.results);
}

} // namespace parrotfish
