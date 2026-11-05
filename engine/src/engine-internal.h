// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Helpers shared by the engine's source files (engine.cpp, search.cpp,
// decide.cpp); not part of the library's interface.

#pragma once

#include "parrotfish/utf8.h"

#include <string>

namespace parrotfish {
namespace internal {

// Longer input (a pasted string, a URL) is left as typed.
constexpr size_t MaxTaps = 40;

inline bool isWordChar(char32_t c)
{
    return utf8::toLower(c) != utf8::toUpper(c) || c == U'ß'
        || (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7);
}

inline std::u32string lower(std::u32string s)
{
    for (char32_t &c : s)
        c = utf8::toLower(c);
    return s;
}

} // namespace internal
} // namespace parrotfish
