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
    m_punctuation.assign(n, 0);
    m_ruleOutput.assign(n, 0);
    for (uint32_t i = 0; i < n; ++i) {
        const format::AlphabetEntry &l = dict.letter(i);
        m_keyOfLetter[i] = keys.indexOf(l.lower);
        m_keyOfBase[i] = keys.indexOf(l.base);
        m_baseOf[l.lower] = l.base;
        m_punctuation[i] = l.cp == U'\'' || l.cp == U'-' || l.cp == U'.';
        for (uint32_t r = 0; r < dict.ruleCount(); ++r)
            if (dict.rules()[r].out == l.lower)
                m_ruleOutput[i] = 1;
    }
}

char32_t Corrector::baseOf(char32_t ch) const
{
    auto it = m_baseOf.find(ch);
    return it == m_baseOf.end() ? ch : it->second;
}

struct Corrector::Search
{
    const Corrector &c;
    const Dictionary &d;
    const CorrectorParams &p;
    const std::vector<Tap> &taps;
    double logPrior;
    size_t n;
    uint32_t a;                      // alphabet size
    std::vector<float> tapCost;      // n × alphabet
    std::vector<double> suffixMin;   // lower bound for taps i..n-1
    std::vector<float> insertCost;   // per tap: skipping it (less if it repeats the last tap)
    double literalCost = 0;          // the taps read exactly as typed

    // For visit(), from prepare().
    std::vector<uint8_t> changed;    // n × alphabet: a different letter, not just its accent
    double wordCosts[256];           // wordCost() per frequency
    // Most taps that spelling rules can take from taps i..n-1 beyond one per
    // letter (ae → ä takes two taps for one letter).
    std::vector<int> ruleSaving;

    std::u32string path;
    std::vector<Candidate> results;
    size_t visits = 0;

    Search(const Corrector &c, const std::vector<Tap> &taps, double logPrior)
        : c(c), d(c.m_dict), p(c.m_params), taps(taps), logPrior(logPrior), n(taps.size()),
          a(d.alphabetSize())
    {
        tapCost.assign(n * a, Inf);
        for (size_t i = 0; i < n; ++i) {
            const Tap &t = taps[i];
            const int k = c.m_keys.indexOf(t.ch);
            const int k0 = t.initial && t.initial != t.ch ? c.m_keys.indexOf(t.initial) : -1;
            const char32_t tbase = c.baseOf(t.ch);
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
        for (size_t i = 0; i < n; ++i) {
            for (uint32_t li = 0; li < a; ++li) {
                if (d.letter(li).lower == taps[i].ch) {
                    literalCost += std::min(tapCost[i * a + li], p.farTap);
                    break;
                }
            }
        }
        insertCost.assign(n, p.insertion);
        for (size_t i = 1; i < n; ++i)
            if (taps[i].ch == taps[i - 1].ch)
                insertCost[i] = std::min(p.insertion, p.doubledInsertion);
        suffixMin.assign(n + 1, 0.0);
        for (size_t i = n; i-- > 0;) {
            float m = insertCost[i];
            for (uint32_t li = 0; li < a; ++li)
                m = std::min(m, tapCost[i * a + li]);
            suffixMin[i] = suffixMin[i + 1] + m;
        }
    }

    void prepare()
    {
        changed.assign(n * a, 0);
        for (size_t i = 0; i < n; ++i) {
            const char32_t tbase = c.baseOf(taps[i].ch);
            for (uint32_t li = 0; li < a; ++li) {
                const format::AlphabetEntry &l = d.letter(li);
                changed[i * a + li] = l.lower != taps[i].ch && l.base != tbase;
            }
        }
        for (int f = 0; f < 256; ++f)
            wordCosts[f] = -(p.lmWeight * d.logProb(static_cast<uint8_t>(f)) + logPrior);
        ruleSaving.assign(n + 1, 0);
        for (size_t i = n; i-- > 0;) {
            ruleSaving[i] = ruleSaving[i + 1];
            for (uint32_t r = 0; r < d.ruleCount(); ++r) {
                size_t len;
                if (ruleMatches(d.rules()[r], i, &len))
                    ruleSaving[i] = std::max(ruleSaving[i], static_cast<int>(len) - 1 + ruleSaving[i + len]);
            }
        }
    }

    float tc(size_t i, uint32_t node) const
    {
        return tapCost[i * a + d.node(node).ch];
    }

    double wordCost(uint8_t freq) const
    {
        return wordCosts[freq];
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

    void emit(uint32_t node, double cost, int edits, int extra, int changes)
    {
        std::u32string target;
        const uint32_t attrs = d.attributes(node, &target);
        const double wc = wordCost(d.node(node).freq);
        Candidate cand;
        cand.freq = d.node(node).freq;
        cand.tapCost = cost;
        cand.edits = edits;
        cand.changes = changes;
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
            extraCand.freq = tn ? d.node(tn).freq : DefaultTargetFreq;
            extraCand.wordCost = wordCost(extraCand.freq);
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

    void visit(uint32_t node, size_t i, double cost, int edits, int extra, int changes)
    {
        if (++visits > MaxVisits)
            return;
        // Too few letters below for the taps still to read, even with every
        // extra tap left skipped and every rule taking its taps: no word.
        if (d.node(node).height < static_cast<int>(n - i) - (p.maxEdits - edits) - ruleSaving[i])
            return;
        if (i == n && node != 0 && d.isTerminal(node) && cost + wordCost(d.node(node).freq) < worst())
            emit(node, cost, edits, extra, changes);

        // An extra tap: skip it.
        if (i < n && edits < p.maxEdits)
            visit(node, i + 1, cost + insertCost[i], edits + 1, extra, changes + 1);

        uint32_t ch = d.firstChild(node);
        if (ch == 0)
            return;
        // No child can beat the worst result: the loop below would stop at
        // the first one (a child's `best` is at most its parent's).
        if (node != 0 && cost + suffixMin[i] + wordCost(d.node(node).best) >= worst())
            return;
        const bool canEdit = edits < p.maxEdits;
        const bool complete = i == n && p.completions && edits == 0
                              && cost <= literalCost + p.completionSlack;
        // With the edits used up, only a close tap, a rule or a skipped ' - .
        // lead on: the other children are passed over (they would do nothing,
        // and the bound below grows along the siblings, so passing over them
        // doesn't change where the loop stops).
        const bool everyChild = canEdit || complete;
        const float *row = i < n ? &tapCost[i * a] : nullptr;
        for (;; ++ch) {
            const format::Node &cn = d.node(ch);
            if (!everyChild && !(row && row[cn.ch] <= p.farTap) && !c.m_punctuation[cn.ch]
                    && !c.m_ruleOutput[cn.ch]) {
                if (d.isLastSibling(ch))
                    break;
                continue;
            }
            const double bound = cost + suffixMin[i] + wordCost(cn.best);
            if (bound >= worst())
                break;   // siblings are sorted by `best`: the rest is worse
            const format::AlphabetEntry &l = d.letter(cn.ch);
            path.push_back(l.cp);

            if (row) {
                const float t = row[cn.ch];
                if (t <= p.farTap)
                    visit(ch, i + 1, cost + t, edits, extra, changes + changed[i * a + cn.ch]);
                else if (t < Inf && canEdit)
                    visit(ch, i + 1, cost + t, edits + 1, extra, changes + 1);
            } else if (complete) {
                visit(ch, i, cost + p.completionCost, edits, extra + 1, changes);
            }

            if (c.m_ruleOutput[cn.ch]) {
                for (uint32_t r = 0; r < d.ruleCount(); ++r) {
                    const format::Rule &rule = d.rules()[r];
                    size_t len;
                    if (rule.out == l.lower && ruleMatches(rule, i, &len))
                        visit(ch, i + len, cost + p.ruleCost, edits, extra, changes);
                }
            }

            if (c.m_punctuation[cn.ch]) {
                visit(ch, i, cost + p.punctuationSkipped, edits, extra, changes);
            } else if (canEdit && (i < n || !p.completions)) {
                // A letter the user left out.
                const bool doubled = path.size() >= 2 && path[path.size() - 2] == l.cp;
                visit(ch, i, cost + (doubled ? std::min(p.deletion, p.doubledDeletion) : p.deletion),
                      edits + 1, extra, changes + 1);
            }

            // Two taps swapped: the next tap is this letter, this tap the one after.
            if (i + 1 < n && canEdit) {
                const float t1 = tc(i + 1, ch);
                uint32_t g = d.firstChild(ch);
                if (t1 < Inf && g != 0) {
                    for (;; ++g) {
                        const float t0 = tc(i, g);
                        if (t0 < Inf) {
                            path.push_back(d.letter(d.node(g).ch).cp);
                            visit(g, i + 2, cost + p.transposition + t0 + t1, edits + 1, extra, changes + 1);
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

double Corrector::matchCost(const std::vector<Tap> &taps, const std::u32string &word, int *edits,
                            int *changes) const
{
    constexpr double inf = std::numeric_limits<double>::infinity();
    if (taps.empty() || word.empty())
        return inf;
    Search s(*this, taps, 0.0);
    const uint32_t a = m_dict.alphabetSize();
    // Alphabet index of each letter of the word.
    std::vector<int> letters;
    for (char32_t c : word) {
        int found = -1;
        for (uint32_t li = 0; li < a && found < 0; ++li)
            if (m_dict.letter(li).cp == static_cast<uint32_t>(c))
                found = static_cast<int>(li);
        if (found < 0)
            return inf;
        letters.push_back(found);
    }
    const size_t n = taps.size(), m = letters.size();
    auto tc = [&](size_t i, size_t j) -> double {
        const float t = s.tapCost[i * a + letters[j]];
        return t <= m_params.farTap ? t : inf;
    };
    // Damerau-Levenshtein over taps × letters with the tap costs; the edit
    // count rides along with the cost of each cell.
    struct Cell { double cost; int edits; int changes; };
    std::vector<Cell> d((n + 1) * (m + 1), Cell{inf, 0, 0});
    auto at = [&](size_t i, size_t j) -> Cell & { return d[i * (m + 1) + j]; };
    at(0, 0) = {0.0, 0, 0};
    for (size_t i = 0; i <= n; ++i) {
        for (size_t j = 0; j <= m; ++j) {
            Cell &c = at(i, j);
            auto relax = [&](const Cell &from, double add, int e, int ch) {
                if (from.cost + add < c.cost && from.edits + e <= m_params.maxEdits)
                    c = {from.cost + add, from.edits + e, from.changes + ch};
            };
            if (i > 0 && j > 0) {
                const format::AlphabetEntry &l = m_dict.letter(letters[j - 1]);
                const int changed = (l.lower != taps[i - 1].ch && l.base != baseOf(taps[i - 1].ch)) ? 1 : 0;
                relax(at(i - 1, j - 1), tc(i - 1, j - 1), 0, changed);
            }
            if (i > 0)
                relax(at(i - 1, j), s.insertCost[i - 1], 1, 1);
            if (j > 0) {
                const char32_t letter = m_dict.letter(letters[j - 1]).cp;
                const bool punct = letter == U'\'' || letter == U'-' || letter == U'.';
                const bool doubled = j > 1 && letters[j - 1] == letters[j - 2];
                relax(at(i, j - 1), punct ? m_params.punctuationSkipped
                                          : doubled ? std::min(m_params.deletion, m_params.doubledDeletion)
                                                    : m_params.deletion,
                      punct ? 0 : 1, punct ? 0 : 1);
            }
            if (i > 1 && j > 1)
                relax(at(i - 2, j - 2), m_params.transposition + tc(i - 1, j - 2) + tc(i - 2, j - 1), 1, 1);
        }
    }
    if (edits)
        *edits = at(n, m).edits;
    if (changes)
        *changes = at(n, m).changes;
    return at(n, m).cost;
}

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
        {"doubledDeletion", &p.doubledDeletion, nullptr, nullptr},
        {"doubledInsertion", &p.doubledInsertion, nullptr, nullptr},
        {"punctuationSkipped", &p.punctuationSkipped, nullptr, nullptr},
        {"ruleCost", &p.ruleCost, nullptr, nullptr},
        {"lmWeight", &p.lmWeight, nullptr, nullptr},
        {"completionCost", &p.completionCost, nullptr, nullptr},
        {"completionSlack", &p.completionSlack, nullptr, nullptr},
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
    s.prepare();
    s.visit(0, 0, 0.0, 0, 0, 0);
    return std::move(s.results);
}

} // namespace parrotfish
