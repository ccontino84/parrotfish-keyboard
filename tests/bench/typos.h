// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Typo generator shared by the benchmarks. The noise is deliberately
// simpler than the engine's model (a uniformly chosen adjacent key), so the
// tests don't just measure the model against itself. Adjacent keys are those
// within 1.25 keys across and up/down, in key widths across and key heights
// up and down, whatever the engine's tap model.

#pragma once

#include "parrotfish/dictionary.h"
#include "parrotfish/keymodel.h"
#include "parrotfish/utf8.h"

#include <map>
#include <random>
#include <string>
#include <vector>

namespace bench {

// DoubleMissed (aparte for apparte) and DoubleExtra (coomputer) only with
// the benches' -D: the other kinds already include some of them.
enum Kind { Clean, Neighbour, Deletion, Insertion, Transposition, Accent, Double, DoubleMissed,
            DoubleExtra, KindCount };
inline const char *kindName(int k)
{
    static const char *names[] = {"clean", "neighbour", "deletion", "insertion", "transposition",
                                  "accent-less", "two typos", "double missed", "tapped twice"};
    return names[k];
}

inline std::u32string lower(std::u32string s)
{
    for (char32_t &c : s)
        c = parrotfish::utf8::toLower(c);
    return s;
}

inline bool isLetter(char32_t c)
{
    return parrotfish::utf8::toLower(c) != parrotfish::utf8::toUpper(c) || c == U'ß';
}

class Typos
{
public:
    Typos(const parrotfish::KeyModel &keys, const parrotfish::Dictionary &dict, unsigned seed)
        : m_rng(seed)
    {
        const parrotfish::KeyModel grid(keys.keys(), keys.sigma(), 1.0f);
        for (size_t k = 0; k < grid.keys().size(); ++k)
            for (int n : grid.neighbours(static_cast<int>(k)))
                m_adjacent[grid.keys()[k].ch].push_back(grid.keys()[n].ch);
        for (uint32_t i = 0; i < dict.alphabetSize(); ++i)
            m_base[dict.letter(i).cp] = dict.letter(i).base;
    }

    std::mt19937 &rng() { return m_rng; }
    size_t pos(size_t n) { return std::uniform_int_distribution<size_t>(0, n - 1)(m_rng); }

    // Lowercase, accents removed (ß → ss).
    std::u32string stripAccents(const std::u32string &w) const
    {
        std::u32string out;
        for (char32_t c : w) {
            auto it = m_base.find(c);
            const char32_t b = it == m_base.end() ? parrotfish::utf8::toLower(c) : it->second;
            if (b == U'ß')
                out += U"ss";
            else
                out.push_back(b);
        }
        return out;
    }
    bool hasAccents(const std::u32string &w) const { return stripAccents(w) != lower(w); }

    // `w` with one typo of `kind` (lowercase); empty if not possible.
    std::u32string apply(std::u32string w, Kind kind)
    {
        if (kind == Accent)
            return hasAccents(w) ? stripAccents(w) : std::u32string();
        if (kind == Double) {
            if (w.size() < 5)
                return {};
            std::u32string t = apply(w, Kind(Neighbour + pos(4)));
            return t.empty() ? t : apply(t, Kind(Neighbour + pos(4)));
        }
        w = lower(w);
        switch (kind) {
        case Neighbour:
            for (int tries = 0; tries < 10; ++tries) {
                const size_t i = pos(w.size());
                auto it = m_adjacent.find(w[i]);
                if (it == m_adjacent.end())
                    continue;
                w[i] = it->second[pos(it->second.size())];
                return w;
            }
            return {};
        case Deletion:
            w.erase(pos(w.size()), 1);
            return w;
        case Insertion: {
            const size_t i = pos(w.size());
            auto it = m_adjacent.find(w[i]);
            const char32_t c = (it == m_adjacent.end() || pos(2) == 0) ? w[i]
                                                                      : it->second[pos(it->second.size())];
            w.insert(w.begin() + i + pos(2), c);
            return w;
        }
        case DoubleMissed: {
            std::vector<size_t> at;
            for (size_t i = 0; i + 1 < w.size(); ++i)
                if (w[i] == w[i + 1] && isLetter(w[i]))
                    at.push_back(i);
            if (at.empty())
                return {};
            w.erase(at[pos(at.size())], 1);
            return w;
        }
        case DoubleExtra: {
            const size_t i = pos(w.size());
            if (!isLetter(w[i]))
                return {};
            w.insert(w.begin() + i, w[i]);
            return w;
        }
        case Transposition: {
            if (w.size() < 3)
                return {};
            const size_t i = pos(w.size() - 1);
            if (w[i] == w[i + 1])
                return {};
            std::swap(w[i], w[i + 1]);
            return w;
        }
        default:
            return w;
        }
    }

private:
    std::mt19937 m_rng;
    std::map<char32_t, std::vector<char32_t>> m_adjacent;
    std::map<char32_t, char32_t> m_base;
};

} // namespace bench
