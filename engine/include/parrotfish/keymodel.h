// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <string>
#include <vector>

namespace parrotfish {

// One character key of the current layout, in any consistent unit.
struct Key {
    char32_t ch;     // lowercase character the key types
    float x, y;      // centre
    float width, height;
};

// The keys of the current layout and how likely a tap on one key was
// meant for another. Taps are modelled per key (handlers don't see touch
// coordinates): the probability falls off with the distance between key
// centres as a 2-D Gaussian, measured in key widths.
class KeyModel
{
public:
    KeyModel() = default;
    explicit KeyModel(std::vector<Key> keys, float sigma = 0.5f);

    // Rows of characters, e.g. {"qwertyuiop", "asdfghjkl", "zxcvbnm"}, with
    // each row's left offset in key widths and its key width (1 = a full key).
    static KeyModel fromRows(const std::vector<std::u32string> &rows,
                             const std::vector<float> &offsets,
                             const std::vector<float> &widths,
                             float sigma = 0.5f);
    // Text form: one row per line, `<offset> <width> <characters>`; `#` comments.
    static KeyModel parse(const std::string &text, std::string *error = nullptr,
                          float sigma = 0.5f);

    const std::vector<Key> &keys() const { return m_keys; }
    int indexOf(char32_t ch) const;   // -1 if no key types `ch`

    // P(the tap landed on key `tapped` | the user aimed at key `intended`).
    float probability(int tapped, int intended) const { return m_prob[tapped * m_keys.size() + intended]; }

    // Keys whose centres are within `radius` key widths of key `k` (not `k`).
    std::vector<int> neighbours(int k, float radius = 1.25f) const;

    float sigma() const { return m_sigma; }

private:
    std::vector<Key> m_keys;
    std::vector<float> m_prob;
    float m_sigma = 0.5f;
};

} // namespace parrotfish
