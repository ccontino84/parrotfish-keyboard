// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Helpers shared by the engine's source files; not part of the library's
// interface.

#pragma once

#include "parrotfish/utf8.h"

#include <string>

namespace parrotfish {
namespace internal {

// Longer input (a pasted string, a URL) is left as typed.
constexpr size_t MaxTaps = 40;

inline bool isGreekLetter(char32_t c)
{
    if (c >= 0x370 && c <= 0x3FF)   // not the numeral signs, ; · and the spacing accents
        return c != 0x374 && c != 0x375 && c != 0x37E && c != 0x384 && c != 0x385 && c != 0x387 && c != 0x3F6;
    if (c >= 0x1F00 && c <= 0x1FFF)   // polytonic, not its spacing accents
        return c != 0x1FBD && !(c >= 0x1FBF && c <= 0x1FC1) && !(c >= 0x1FCD && c <= 0x1FCF)
            && !(c >= 0x1FDD && c <= 0x1FDF) && !(c >= 0x1FED && c <= 0x1FEF) && c < 0x1FFD;
    return false;
}

inline bool isWordChar(char32_t c)
{
    return utf8::toLower(c) != utf8::toUpper(c) || c == U'ß'
        || (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7)
        || isGreekLetter(c)
        || (c >= 0x400 && c <= 0x52F && !(c >= 0x482 && c <= 0x489));   // not ҂ and the combining marks
}

// A letter in a word in capitals: Greek drops the tonos there (ΚΑΛΑ for
// καλά) but keeps the dialytika (ΐ: Ϊ).
inline char32_t upperInCapitals(char32_t c)
{
    switch (c) {
    case 0x3AC: case 0x386: return 0x391;
    case 0x3AD: case 0x388: return 0x395;
    case 0x3AE: case 0x389: return 0x397;
    case 0x3AF: case 0x38A: return 0x399;
    case 0x3CC: case 0x38C: return 0x39F;
    case 0x3CD: case 0x38E: return 0x3A5;
    case 0x3CE: case 0x38F: return 0x3A9;
    case 0x390: return 0x3AA;
    case 0x3B0: return 0x3AB;
    default: return utf8::toUpper(c);
    }
}

inline std::u32string lower(std::u32string s)
{
    for (char32_t &c : s)
        c = utf8::toLower(c);
    return s;
}

} // namespace internal
} // namespace parrotfish
