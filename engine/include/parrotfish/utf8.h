// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

namespace parrotfish {
namespace utf8 {

std::u32string decode(const std::string &s);
std::string encode(const std::u32string &s);
std::string encode(char32_t c);

// Simple case mapping for the Latin, Greek and Cyrillic letters the
// layouts produce. Characters outside those blocks are returned unchanged.
char32_t toLower(char32_t c);
char32_t toUpper(char32_t c);
bool isUpper(char32_t c);

// The plain letter of a lowercase Latin letter with an accent (à → a,
// ø → o), as tools/data/build-dict.py's base_char; else `c`. For letters
// no loaded dictionary has.
char32_t latinBase(char32_t c);

} // namespace utf8
} // namespace parrotfish
