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
// centres as a 2-D Gaussian. Distances across are in key widths; distances
// up and down in a mix of the key's width and height: heightShare 0 = key
// widths too (the engine's), 1 = key heights (the benches' typo noise).
class KeyModel
{
public:
    KeyModel() = default;
    explicit KeyModel(std::vector<Key> keys, float sigma = 0.5f, float heightShare = 0.0f);

    // Rows of characters, e.g. {"qwertyuiop", "asdfghjkl", "zxcvbnm"}, with
    // each row's left offset in key widths and its key width (1 = a full key);
    // rows are `pitch` apart and keys `pitch` tall.
    static KeyModel fromRows(const std::vector<std::u32string> &rows,
                             const std::vector<float> &offsets,
                             const std::vector<float> &widths,
                             float sigma = 0.5f, float pitch = 1.0f);
    // Text form: one row per line, `<offset> <width> <characters>`; `#` comments;
    // an optional line `pitch <p>` sets the row distance and key height
    // (default 1, in the same unit).
    static KeyModel parse(const std::string &text, std::string *error = nullptr,
                          float sigma = 0.5f);

    const std::vector<Key> &keys() const { return m_keys; }
    int indexOf(char32_t ch) const;   // -1 if no key types `ch`

    // P(the tap landed on key `tapped` | the user aimed at key `intended`).
    float probability(int tapped, int intended) const { return m_prob[tapped * m_keys.size() + intended]; }

    // Keys whose centres are within `radius` of key `k` (not `k`), in the
    // units of the tap model.
    std::vector<int> neighbours(int k, float radius = 1.25f) const;

    float sigma() const { return m_sigma; }
    float heightShare() const { return m_heightShare; }

private:
    std::vector<Key> m_keys;
    std::vector<float> m_prob;
    float m_sigma = 0.5f;
    float m_heightShare = 0.0f;
    float yUnit(const Key &k) const;
};

} // namespace parrotfish
