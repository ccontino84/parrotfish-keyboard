// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "parrotfish/keymodel.h"
#include "parrotfish/utf8.h"

#include <cmath>
#include <sstream>

namespace parrotfish {

KeyModel::KeyModel(std::vector<Key> keys, float sigma, float heightShare)
    : m_keys(std::move(keys))
    , m_sigma(sigma)
    , m_heightShare(heightShare)
{
    const size_t n = m_keys.size();
    m_prob.assign(n * n, 0.0f);
    // Column `intended` holds the distribution over tapped keys.
    for (size_t intended = 0; intended < n; ++intended) {
        const Key &a = m_keys[intended];
        double unit = a.width > 0 ? a.width : 1.0;
        double vunit = yUnit(a);
        double sum = 0.0;
        std::vector<double> w(n);
        for (size_t tapped = 0; tapped < n; ++tapped) {
            const Key &b = m_keys[tapped];
            double dx = (b.x - a.x) / unit;
            double dy = (b.y - a.y) / vunit;
            w[tapped] = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma));
            sum += w[tapped];
        }
        for (size_t tapped = 0; tapped < n; ++tapped)
            m_prob[tapped * n + intended] = static_cast<float>(w[tapped] / sum);
    }
}

KeyModel KeyModel::fromRows(const std::vector<std::u32string> &rows,
                            const std::vector<float> &offsets,
                            const std::vector<float> &widths, float sigma, float pitch)
{
    std::vector<Key> keys;
    for (size_t r = 0; r < rows.size(); ++r) {
        float w = r < widths.size() ? widths[r] : 1.0f;
        float x = r < offsets.size() ? offsets[r] : 0.0f;
        for (char32_t ch : rows[r]) {
            keys.push_back({ch, x + w / 2, (static_cast<float>(r) + 0.5f) * pitch, w, pitch});
            x += w;
        }
    }
    return KeyModel(std::move(keys), sigma);
}

KeyModel KeyModel::parse(const std::string &text, std::string *error, float sigma)
{
    std::vector<std::u32string> rows;
    std::vector<float> offsets, widths;
    float pitch = 1.0f;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        line = line.substr(0, line.find('#'));
        std::istringstream ls(line);
        std::string first;
        if (!(ls >> first))
            continue;
        if (first == "pitch") {
            ls >> pitch;
            continue;
        }
        float offset, width;
        std::string chars;
        ls.clear();
        ls.seekg(0);
        if (!(ls >> offset))
            continue;
        if (!(ls >> width >> chars)) {
            if (error)
                *error = "bad layout row: " + line;
            return KeyModel();
        }
        rows.push_back(utf8::decode(chars));
        offsets.push_back(offset);
        widths.push_back(width);
    }
    return fromRows(rows, offsets, widths, sigma, pitch);
}

int KeyModel::indexOf(char32_t ch) const
{
    for (size_t i = 0; i < m_keys.size(); ++i)
        if (m_keys[i].ch == ch)
            return static_cast<int>(i);
    return -1;
}

std::vector<int> KeyModel::neighbours(int k, float radius) const
{
    std::vector<int> out;
    const Key &a = m_keys[k];
    float unit = a.width > 0 ? a.width : 1.0f;
    float vunit = yUnit(a);
    for (size_t i = 0; i < m_keys.size(); ++i) {
        if (static_cast<int>(i) == k)
            continue;
        float dx = (m_keys[i].x - a.x) / unit;
        float dy = (m_keys[i].y - a.y) / vunit;
        if (dx * dx + dy * dy <= radius * radius)
            out.push_back(static_cast<int>(i));
    }
    return out;
}

float KeyModel::yUnit(const Key &k) const
{
    const float w = k.width > 0 ? k.width : 1.0f;
    const float h = k.height > 0 ? k.height : w;
    return w + m_heightShare * (h - w);
}

} // namespace parrotfish
