// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/dictformat.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace parrotfish {

// A read-only, memory-mapped word list (one language).
class Dictionary
{
public:
    Dictionary() = default;
    ~Dictionary();
    Dictionary(const Dictionary &) = delete;
    Dictionary &operator=(const Dictionary &) = delete;
    Dictionary(Dictionary &&other) noexcept;
    Dictionary &operator=(Dictionary &&other) noexcept;

    // Maps the file. Returns false and sets `error` if it is missing or invalid.
    bool open(const std::string &path, std::string *error = nullptr);
    bool isOpen() const { return m_data != nullptr; }

    std::string language() const;
    uint32_t wordCount() const { return m_header->wordCount; }
    uint32_t nodeCount() const { return m_header->nodeCount; }

    // Trie access. Node 0 is the root.
    const format::Node &node(uint32_t i) const { return m_nodes[i]; }
    uint32_t firstChild(uint32_t i) const { return m_nodes[i].link & format::LinkChildMask; }
    bool isTerminal(uint32_t i) const { return m_nodes[i].link & format::LinkTerminal; }
    bool isLastSibling(uint32_t i) const { return m_nodes[i].link & format::LinkLastSibling; }

    uint32_t alphabetSize() const { return m_header->alphabetCount; }
    const format::AlphabetEntry &letter(uint16_t ch) const { return m_alphabet[ch]; }

    const format::Rule *rules() const { return m_rules; }
    uint32_t ruleCount() const { return m_header->ruleCount; }

    // Attributes of a terminal node (0 if none); `target` gets the target
    // word for misspellings and shortcuts.
    uint32_t attributes(uint32_t node, std::u32string *target = nullptr) const;

    // log P(word) in this language for a frequency 0..255.
    double logProb(uint8_t freq) const;

    // Node of the word typed exactly, ignoring case only (0 if absent).
    // An exact-case match wins; `stored` gets the word as stored.
    uint32_t find(const std::u32string &word, std::u32string *stored = nullptr) const;
    // Highest frequency of the words starting with `prefix`, ignoring case
    // (-1 if no word starts with it).
    int prefixBest(const std::u32string &prefix) const;
    // Node of the word as stored, case-sensitive (0 if absent).
    uint32_t findExact(const std::u32string &word) const;

    // Starting next-word data: the words that most often follow `prev` (a
    // terminal node, format::PairStart or format::PairAny), most frequent
    // first, with P(next | prev) on the 0..255 scale. Empty if none.
    std::vector<std::pair<std::u32string, uint8_t>> nextWords(uint32_t prev) const;

    // Calls f(node, word) for every terminal node (tests and tools).
    template<typename F> void forEachWord(F f) const;

private:
    void close();

    const uint8_t *m_data = nullptr;
    size_t m_size = 0;
    const format::Header *m_header = nullptr;
    const format::Node *m_nodes = nullptr;
    const format::AlphabetEntry *m_alphabet = nullptr;
    const format::Attr *m_attrs = nullptr;
    const format::Rule *m_rules = nullptr;
    const char *m_strings = nullptr;
    const format::PairRow *m_pairRows = nullptr;
    const format::PairEntry *m_pairEntries = nullptr;
};

template<typename F>
void Dictionary::forEachWord(F f) const
{
    std::u32string word;
    std::vector<uint32_t> stack{0};
    std::vector<size_t> lengths{0};
    // Depth-first: (node, prefix length) pairs.
    while (!stack.empty()) {
        uint32_t n = stack.back();
        size_t len = lengths.back();
        stack.pop_back();
        lengths.pop_back();
        word.resize(len);
        if (n != 0)
            word.push_back(m_alphabet[m_nodes[n].ch].cp);
        if (isTerminal(n))
            f(n, word);
        uint32_t c = firstChild(n);
        if (c == 0)
            continue;
        for (;; ++c) {
            stack.push_back(c);
            lengths.push_back(word.size());
            if (isLastSibling(c))
                break;
        }
    }
}

} // namespace parrotfish
