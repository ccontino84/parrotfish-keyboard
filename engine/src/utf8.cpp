// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "parrotfish/utf8.h"

#include <algorithm>

namespace parrotfish {
namespace utf8 {

std::u32string decode(const std::string &s)
{
    std::u32string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        unsigned char b = s[i];
        char32_t c;
        size_t n;
        if (b < 0x80) { c = b; n = 1; }
        else if ((b & 0xE0) == 0xC0) { c = b & 0x1F; n = 2; }
        else if ((b & 0xF0) == 0xE0) { c = b & 0x0F; n = 3; }
        else if ((b & 0xF8) == 0xF0) { c = b & 0x07; n = 4; }
        else { out.push_back(0xFFFD); ++i; continue; }
        if (i + n > s.size()) { out.push_back(0xFFFD); break; }
        bool ok = true;
        for (size_t k = 1; k < n; ++k) {
            unsigned char cb = s[i + k];
            if ((cb & 0xC0) != 0x80) { ok = false; break; }
            c = (c << 6) | (cb & 0x3F);
        }
        if (!ok) { out.push_back(0xFFFD); ++i; continue; }
        out.push_back(c);
        i += n;
    }
    return out;
}

std::string encode(char32_t c)
{
    std::string out;
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
    return out;
}

std::string encode(const std::u32string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char32_t c : s)
        out += encode(c);
    return out;
}

char32_t toLower(char32_t c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    if (c < 0xC0)
        return c;
    if (c <= 0xDE && c != 0xD7)
        return c + 32;
    if (c >= 0x100 && c <= 0x137 && c != 0x130)
        return c | 1;
    if (c >= 0x139 && c <= 0x148)
        return (c & 1) ? c + 1 : c;
    if (c >= 0x14A && c <= 0x177)
        return c | 1;
    if (c == 0x178)
        return 0xFF;
    if (c >= 0x179 && c <= 0x17E)
        return (c & 1) ? c + 1 : c;
    if (c < 0x386)
        return c;
    if (c == 0x386)
        return 0x3AC;
    if (c >= 0x388 && c <= 0x38A)
        return c + 37;
    if (c == 0x38C)
        return 0x3CC;
    if (c == 0x38E || c == 0x38F)
        return c + 63;
    if (c >= 0x391 && c <= 0x3AB && c != 0x3A2)
        return c + 32;
    if (c >= 0x410 && c <= 0x42F)
        return c + 32;
    if (c >= 0x400 && c <= 0x40F)
        return c + 80;
    if ((c >= 0x460 && c <= 0x481) || (c >= 0x48A && c <= 0x4BF) || (c >= 0x4D0 && c <= 0x52F))
        return c | 1;
    if (c == 0x4C0)
        return 0x4CF;
    if (c >= 0x4C1 && c <= 0x4CE)
        return (c & 1) ? c + 1 : c;
    return c;
}

char32_t toUpper(char32_t c)
{
    if (c >= 'a' && c <= 'z')
        return c - 32;
    if (c < 0xE0)
        return c;
    if (c <= 0xFE && c != 0xF7)
        return c - 32;
    if (c == 0xFF)
        return 0x178;
    if (c >= 0x100 && c <= 0x137 && c != 0x131)
        return c & ~char32_t(1);
    if (c >= 0x139 && c <= 0x148)
        return (c & 1) ? c : c - 1;
    if (c >= 0x14A && c <= 0x177)
        return c & ~char32_t(1);
    if (c >= 0x179 && c <= 0x17E)
        return (c & 1) ? c : c - 1;
    if (c < 0x3AC)
        return c;
    if (c == 0x3AC)
        return 0x386;
    if (c >= 0x3AD && c <= 0x3AF)
        return c - 37;
    if (c == 0x3C2)
        return 0x3A3;   // final sigma
    if (c >= 0x3B1 && c <= 0x3CB)
        return c - 32;
    if (c == 0x3CC)
        return 0x38C;
    if (c == 0x3CD || c == 0x3CE)
        return c - 63;
    if (c >= 0x430 && c <= 0x44F)
        return c - 32;
    if (c >= 0x450 && c <= 0x45F)
        return c - 80;
    if ((c >= 0x460 && c <= 0x481) || (c >= 0x48A && c <= 0x4BF) || (c >= 0x4D0 && c <= 0x52F))
        return c & ~char32_t(1);
    if (c == 0x4CF)
        return 0x4C0;
    if (c >= 0x4C1 && c <= 0x4CE)
        return (c & 1) ? c : c - 1;
    return c;
}

bool isUpper(char32_t c)
{
    return toLower(c) != c;
}

char32_t latinBase(char32_t c)
{
    // Sorted; generated with unicodedata (NFD) from U+00E0-U+017F.
    static const char32_t accented[] =
        U"àáâãäåæçèéêëìíîïñòóôõöøùúûüýÿāăąćĉċčďēĕėęěĝğġģĥĩīĭįĵķĺļľńņňōŏőœŕŗřśŝşšţťũūŭůűųŵŷźżž";
    static const char32_t base[] =
        U"aaaaaaaceeeeiiiinoooooouuuuyyaaaccccdeeeeegggghiiiijklllnnnoooorrrssssttuuuuuuwyzzz";
    static_assert(sizeof(accented) == sizeof(base), "one base per letter");
    const char32_t *end = accented + sizeof(accented) / sizeof(char32_t) - 1;
    const char32_t *it = std::lower_bound(accented, end, c);
    return it != end && *it == c ? base[it - accented] : c;
}

} // namespace utf8
} // namespace parrotfish
