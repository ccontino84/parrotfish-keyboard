// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// On-disk layout of a Parrotfish dictionary (.pfd), written by
// tools/data/build-dict.py. Little-endian; every section is 8-byte aligned.
// Keep FormatVersion in sync with FORMAT_VERSION in the builder.

#pragma once

#include <cstdint>

namespace parrotfish {
namespace format {

constexpr char Magic[8] = {'P', 'F', 'D', 'I', 'C', 'T', 0, 0};
constexpr uint32_t FormatVersion = 3;

struct Header {
    char magic[8];
    uint32_t version;
    uint32_t headerSize;
    char lang[8];
    uint32_t wordCount;     // real words (misspellings excluded)
    float logNorm;          // log of the probability mass of real words
    uint32_t nodeCount, nodesOffset;
    uint32_t alphabetCount, alphabetOffset;
    uint32_t attrCount, attrOffset;
    uint32_t ruleCount, ruleOffset;
    uint32_t stringsSize, stringsOffset;
    uint32_t pairRowCount, pairRowOffset;
    uint32_t pairEntryCount, pairEntryOffset;
    // The `suffix` and `compound` lines of the language's rules, one per
    // line (Dictionary::isWordForm): 1 + their offset in the strings
    // section, or 0 for none (files made before them).
    uint32_t wordParts;
    uint32_t reserved;
};

// Trie node. Children of a node are contiguous, sorted by `best`
// (descending). Node 0 is the root.
struct Node {
    uint32_t link;          // bits 0-29: first child (0 = none)
    uint8_t ch;             // index into the alphabet
    uint8_t height;         // most letters below: longest word minus depth (at most 255)
    uint8_t best;           // highest frequency in this subtree
    uint8_t freq;           // own frequency, if terminal
};
constexpr uint32_t LinkChildMask = (1u << 30) - 1;
constexpr uint32_t LinkTerminal = 1u << 30;
constexpr uint32_t LinkLastSibling = 1u << 31;

struct AlphabetEntry {
    uint32_t cp;            // the character as stored
    uint32_t lower;         // lowercase form
    uint32_t base;          // lowercase without accents (é → e, ø → o, æ → a, œ → o; ß stays ß)
};

enum AttrFlag : uint32_t {
    AttrOffensive = 1u << 0,    // valid, never suggested unprompted
    AttrMisspelling = 1u << 1,  // not a word: corrected to `target`
    AttrAbbreviation = 1u << 2,
    AttrNonword = 1u << 3,      // informal (gonna): valid
    AttrShortcut = 1u << 4,     // valid, with an extra suggestion `target`
};

// Sorted by node.
struct Attr {
    uint32_t node;
    uint32_t flags;
    uint32_t target;        // offset into the strings section, or NoTarget
};
constexpr uint32_t NoTarget = 0xFFFFFFFFu;

enum RuleFlag : uint32_t {
    RuleFinal = 1u << 0,    // only at the end of the word
};

// Typed sequence `in` (up to 3 characters) stands for the letter `out`.
struct Rule {
    uint32_t in[3];
    uint32_t out;
    uint32_t flags;
};

// Starting next-word data: for a previous word, its most frequent next
// words. Rows are sorted by `prev` (a terminal node, PairStart or PairAny);
// a row's entries are contiguous and sorted by frequency (descending).
constexpr uint32_t PairStart = 0;            // start of a sentence
constexpr uint32_t PairAny = 0xFFFFFFFFu;    // fallback: frequent words

struct PairRow {
    uint32_t prev;
    uint32_t first;         // index of the first entry
    uint16_t count;
    uint16_t reserved;
};

struct PairEntry {
    uint32_t word;          // offset of the next word in the strings section
    uint8_t freq;           // P(next | prev) on the 0..255 scale
    uint8_t reserved[3];
};

// AOSP frequencies: 255 is probability 1, each step down divides by 1.15.
constexpr double LogStep = 0.13976194237515863;  // log(1.15)

} // namespace format
} // namespace parrotfish
