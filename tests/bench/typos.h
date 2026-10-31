// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Typo generator shared by the benchmarks. The noise is deliberately
// simpler than the engine's model, so the tests don't just measure the model
// against itself. Adjacent keys are those within 1.25 keys across and
// up/down, in key widths across and key heights up and down, whatever the
// engine's tap model; only letter keys and the apostrophe count (not the
// digits of a number row).
//
// A wrong key (Neighbour) goes sideways (same row) or up/down (another
// row): 10% up/down by default (one user's slips.log of 2026-10-31, 117
// wrong keys: 9%), and sideways left or right alike. fitDirections() reads
// both from a slips.log, left and right apart for each half of the keyboard
// (that user: right half 48 left : 12 right, left half 24 : 22), which
// belong to one person's hands: setMirrored() swaps them, as for the other
// hand. setUniform() picks any adjacent key, as before 2026-10-31 (about
// half of them up/down on the phone's QWERTY).
//
// Kinds after RealWord need two words or a number row (sentence-bench -X):
// Joined, two words with the space left out (thisis); SpaceLetter, the space
// typed as a letter key above the spacebar (thisbis); NumberRow, a letter of
// the top row typed as the digit above it (h3llo). Unknown marks a word no
// loaded dictionary knows (a name), typed correctly.

#pragma once

#include "parrotfish/dictionary.h"
#include "parrotfish/keymodel.h"
#include "parrotfish/utf8.h"

#include <cmath>
#include <fstream>
#include <algorithm>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace bench {

// DoubleMissed (aparte for apparte) and DoubleExtra (coomputer) only with
// the benches' -D: the other kinds already include some of them.
// RealWord (form for from) only with sentence-bench -R.
enum Kind { Clean, Neighbour, Deletion, Insertion, Transposition, Accent, Double, DoubleMissed,
            DoubleExtra, RealWord, Joined, SpaceLetter, NumberRow, Unknown, KindCount };
inline const char *kindName(int k)
{
    static const char *names[] = {"clean", "neighbour", "deletion", "insertion", "transposition",
                                  "accent-less", "two typos", "double missed", "tapped twice", "real word",
                                  "no space", "space as key", "number row", "unknown word"};
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
        const std::vector<parrotfish::Key> &ks = grid.keys();
        float minX = 0, maxX = 0;
        for (size_t k = 0; k < ks.size(); ++k) {
            m_key[ks[k].ch] = ks[k];
            minX = k ? std::min(minX, ks[k].x) : ks[k].x;
            maxX = k ? std::max(maxX, ks[k].x) : ks[k].x;
        }
        m_middle = (minX + maxX) / 2;
        for (size_t k = 0; k < ks.size(); ++k) {
            if (!typable(ks[k].ch))
                continue;
            for (int n : grid.neighbours(static_cast<int>(k))) {
                const parrotfish::Key &o = ks[n];
                if (!typable(o.ch))
                    continue;
                m_adjacent[ks[k].ch].push_back(o.ch);
                if (std::fabs(o.y - ks[k].y) > ks[k].height / 2)
                    m_vertical[ks[k].ch].push_back(o.ch);
                else
                    (o.x < ks[k].x ? m_left : m_right)[ks[k].ch].push_back(o.ch);
            }
            // The digit right above a letter of the top row.
            for (const parrotfish::Key &o : ks)
                if (o.ch >= U'0' && o.ch <= U'9' && std::fabs(o.x - ks[k].x) < ks[k].width / 2
                        && o.y < ks[k].y && ks[k].y - o.y < 1.01f * ks[k].height)
                    m_digitAbove[ks[k].ch] = o.ch;
        }
        for (uint32_t i = 0; i < dict.alphabetSize(); ++i)
            m_base[dict.letter(i).cp] = dict.letter(i).base;
    }

    // Any adjacent key for a wrong key, as before 2026-10-31.
    void setUniform(bool uniform) { m_uniform = uniform; }
    // Left and right swapped, halves too (the other hand).
    void setMirrored(bool mirrored) { m_mirrored = mirrored; }

    // The directions of wrong keys from a slips.log ("... key TYPED MEANT"
    // lines, keys of this layout); false if it has none.
    bool fitDirections(const std::string &path)
    {
        std::ifstream in(path);
        std::string line;
        int vertical = 0, sideways = 0, left[2] = {0, 0}, total[2] = {0, 0};
        while (std::getline(in, line)) {
            std::istringstream ls(line);
            std::vector<std::string> f;
            for (std::string w; ls >> w;)
                f.push_back(w);
            auto it = std::find(f.begin(), f.end(), "key");
            if (f.empty() || f[0] == "#" || it == f.end() || f.end() - it < 3)
                continue;
            const std::u32string t = parrotfish::utf8::decode(*(it + 1)), m = parrotfish::utf8::decode(*(it + 2));
            if (t.size() != 1 || m.size() != 1 || !m_key.count(t[0]) || !m_key.count(m[0]))
                continue;
            const parrotfish::Key &kt = m_key[t[0]], &km = m_key[m[0]];
            if (std::fabs(kt.y - km.y) > km.height / 2) {
                ++vertical;
            } else {
                ++sideways;
                const int half = km.x > m_middle ? 1 : 0;
                ++total[half];
                if (kt.x < km.x)
                    ++left[half];
            }
        }
        if (vertical + sideways == 0)
            return false;
        m_verticalShare = double(vertical) / (vertical + sideways);
        for (int h = 0; h < 2; ++h)
            if (total[h])
                m_leftShare[h] = double(left[h]) / total[h];
        return true;
    }
    double verticalShare() const { return m_verticalShare; }
    double leftShare(bool rightHalf) const { return m_leftShare[rightHalf ? 1 : 0]; }

    // Whether the layout has a number row (NumberRow typos are possible).
    bool hasDigits() const { return !m_digitAbove.empty(); }

    // `w` with a letter of the top row typed as the digit above it; empty if
    // it has none.
    std::u32string numberRowSlip(std::u32string w)
    {
        w = lower(w);
        std::vector<size_t> at;
        for (size_t i = 0; i < w.size(); ++i)
            if (m_digitAbove.count(w[i]))
                at.push_back(i);
        if (at.empty())
            return {};
        const size_t i = at[pos(at.size())];
        w[i] = m_digitAbove[w[i]];
        return w;
    }

    // A wrong key for `c`, in the directions of real slips (0 if none).
    char32_t wrongKey(char32_t c)
    {
        auto pick = [this](const std::map<char32_t, std::vector<char32_t>> &m, char32_t c) -> char32_t {
            auto it = m.find(c);
            return it == m.end() || it->second.empty() ? 0 : it->second[pos(it->second.size())];
        };
        if (m_uniform)
            return pick(m_adjacent, c);
        std::uniform_real_distribution<double> coin(0.0, 1.0);
        char32_t out = 0;
        if (coin(m_rng) < m_verticalShare)
            out = pick(m_vertical, c);
        if (!out) {
            const bool rightHalf = (m_key.count(c) && m_key[c].x > m_middle) != m_mirrored;
            const bool goLeft = (coin(m_rng) < m_leftShare[rightHalf ? 1 : 0]) != m_mirrored;
            out = pick(goLeft ? m_left : m_right, c);
            if (!out)
                out = pick(goLeft ? m_right : m_left, c);
        }
        return out ? out : pick(m_adjacent, c);
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

    // `w` (lowercase) with one letter changed to an adjacent key or two
    // neighbouring letters swapped, in every way.
    std::vector<std::u32string> oneSlip(std::u32string w) const
    {
        w = lower(w);
        std::vector<std::u32string> out;
        for (size_t i = 0; i < w.size(); ++i) {
            auto it = m_adjacent.find(w[i]);
            if (it != m_adjacent.end())
                for (char32_t c : it->second) {
                    std::u32string t = w;
                    t[i] = c;
                    out.push_back(t);
                }
            if (i + 1 < w.size() && w[i] != w[i + 1]) {
                std::u32string t = w;
                std::swap(t[i], t[i + 1]);
                out.push_back(t);
            }
        }
        return out;
    }

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
                const char32_t c = wrongKey(w[i]);
                if (!c)
                    continue;
                w[i] = c;
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
    static bool typable(char32_t c) { return isLetter(c) || c == U'\''; }

    std::mt19937 m_rng;
    bool m_uniform = false, m_mirrored = false;
    double m_verticalShare = 0.1;
    double m_leftShare[2] = {0.5, 0.5};   // left half, right half
    float m_middle = 0;
    std::map<char32_t, parrotfish::Key> m_key;
    std::map<char32_t, char32_t> m_digitAbove;
    std::map<char32_t, std::vector<char32_t>> m_adjacent, m_vertical, m_left, m_right;
    std::map<char32_t, char32_t> m_base;
};

} // namespace bench
