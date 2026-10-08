// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "parrotfish/dictionary.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace parrotfish {

Dictionary::~Dictionary()
{
    close();
}

Dictionary::Dictionary(Dictionary &&other) noexcept
{
    *this = std::move(other);
}

Dictionary &Dictionary::operator=(Dictionary &&other) noexcept
{
    if (this != &other) {
        close();
        m_data = other.m_data;
        m_size = other.m_size;
        m_header = other.m_header;
        m_nodes = other.m_nodes;
        m_alphabet = other.m_alphabet;
        m_attrs = other.m_attrs;
        m_rules = other.m_rules;
        m_strings = other.m_strings;
        m_pairRows = other.m_pairRows;
        m_pairEntries = other.m_pairEntries;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

void Dictionary::close()
{
    if (m_data)
        munmap(const_cast<uint8_t *>(m_data), m_size);
    m_data = nullptr;
    m_size = 0;
}

static bool fail(std::string *error, const std::string &message)
{
    if (error)
        *error = message;
    return false;
}

bool Dictionary::open(const std::string &path, std::string *error)
{
    close();
    int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return fail(error, path + ": cannot open");
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < static_cast<off_t>(sizeof(format::Header))) {
        ::close(fd);
        return fail(error, path + ": too small");
    }
    void *p = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    ::close(fd);
    if (p == MAP_FAILED)
        return fail(error, path + ": cannot map");
    m_data = static_cast<const uint8_t *>(p);
    m_size = st.st_size;
    m_header = reinterpret_cast<const format::Header *>(m_data);

    const format::Header &h = *m_header;
    auto inRange = [this](uint32_t offset, uint64_t count, size_t size) {
        return offset % 8 == 0 && offset + count * size <= m_size;
    };
    if (std::memcmp(h.magic, format::Magic, 8) != 0 || h.version != format::FormatVersion
            || h.headerSize != sizeof(format::Header)
            || h.nodeCount == 0
            || !inRange(h.nodesOffset, h.nodeCount, sizeof(format::Node))
            || !inRange(h.alphabetOffset, h.alphabetCount, sizeof(format::AlphabetEntry))
            || !inRange(h.attrOffset, h.attrCount, sizeof(format::Attr))
            || !inRange(h.ruleOffset, h.ruleCount, sizeof(format::Rule))
            || !inRange(h.stringsOffset, h.stringsSize, 1)
            || !inRange(h.pairRowOffset, h.pairRowCount, sizeof(format::PairRow))
            || !inRange(h.pairEntryOffset, h.pairEntryCount, sizeof(format::PairEntry))) {
        close();
        return fail(error, path + ": not a Parrotfish dictionary of version "
                    + std::to_string(format::FormatVersion));
    }
    m_nodes = reinterpret_cast<const format::Node *>(m_data + h.nodesOffset);
    m_alphabet = reinterpret_cast<const format::AlphabetEntry *>(m_data + h.alphabetOffset);
    m_attrs = reinterpret_cast<const format::Attr *>(m_data + h.attrOffset);
    m_rules = reinterpret_cast<const format::Rule *>(m_data + h.ruleOffset);
    m_strings = reinterpret_cast<const char *>(m_data + h.stringsOffset);
    m_pairRows = reinterpret_cast<const format::PairRow *>(m_data + h.pairRowOffset);
    m_pairEntries = reinterpret_cast<const format::PairEntry *>(m_data + h.pairEntryOffset);
    if (h.stringsSize > 0 && m_strings[h.stringsSize - 1] != 0) {
        close();
        return fail(error, path + ": damaged string table");
    }
    return true;
}

std::string Dictionary::language() const
{
    return std::string(m_header->lang, strnlen(m_header->lang, sizeof(m_header->lang)));
}

uint32_t Dictionary::attributes(uint32_t node, std::u32string *target) const
{
    const format::Attr *end = m_attrs + m_header->attrCount;
    const format::Attr *a = std::lower_bound(m_attrs, end, node,
        [](const format::Attr &x, uint32_t n) { return x.node < n; });
    if (a == end || a->node != node)
        return 0;
    if (target) {
        target->clear();
        if (a->target != format::NoTarget && a->target < m_header->stringsSize)
            *target = utf8::decode(m_strings + a->target);
    }
    return a->flags;
}

std::vector<std::pair<std::u32string, uint8_t>> Dictionary::nextWords(uint32_t prev) const
{
    std::vector<std::pair<std::u32string, uint8_t>> out;
    const format::PairRow *end = m_pairRows + m_header->pairRowCount;
    const format::PairRow *row = std::lower_bound(m_pairRows, end, prev,
        [](const format::PairRow &r, uint32_t p) { return r.prev < p; });
    if (row == end || row->prev != prev)
        return out;
    for (uint32_t i = row->first; i < row->first + row->count && i < m_header->pairEntryCount; ++i) {
        const format::PairEntry &e = m_pairEntries[i];
        if (e.word < m_header->stringsSize)
            out.emplace_back(utf8::decode(m_strings + e.word), e.freq);
    }
    return out;
}

double Dictionary::logProb(uint8_t freq) const
{
    return (static_cast<int>(freq) - 255) * format::LogStep - m_header->logNorm;
}

uint32_t Dictionary::find(const std::u32string &word, std::u32string *stored) const
{
    // Case-insensitive: there can be several matching paths (Rose, rose),
    // so search all of them and prefer an exact-case match.
    uint32_t best = 0;
    bool bestExact = false;
    struct Item { uint32_t node; std::u32string path; bool exact; };
    std::vector<Item> stack{{0, {}, true}};
    while (!stack.empty()) {
        Item it = std::move(stack.back());
        stack.pop_back();
        const size_t pos = it.path.size();
        if (pos == word.size()) {
            if (it.node != 0 && isTerminal(it.node) && (!best || (it.exact && !bestExact))) {
                best = it.node;
                bestExact = it.exact;
                if (stored)
                    *stored = it.path;
            }
            continue;
        }
        uint32_t c = firstChild(it.node);
        if (c == 0)
            continue;
        const uint32_t want = utf8::toLower(word[pos]);
        for (;; ++c) {
            const format::AlphabetEntry &l = m_alphabet[m_nodes[c].ch];
            if (l.lower == want) {
                Item next{c, it.path, it.exact && l.cp == word[pos]};
                next.path.push_back(l.cp);
                stack.push_back(std::move(next));
            }
            if (isLastSibling(c))
                break;
        }
    }
    return best;
}

int Dictionary::prefixBest(const std::u32string &prefix) const
{
    int best = -1;
    std::vector<std::pair<uint32_t, size_t>> stack{{0, 0}};
    while (!stack.empty()) {
        const auto it = stack.back();
        stack.pop_back();
        if (it.second == prefix.size()) {
            best = std::max<int>(best, it.first == 0 ? 255 : m_nodes[it.first].best);
            continue;
        }
        uint32_t c = firstChild(it.first);
        if (c == 0)
            continue;
        const uint32_t want = utf8::toLower(prefix[it.second]);
        for (;; ++c) {
            if (m_alphabet[m_nodes[c].ch].lower == want)
                stack.push_back({c, it.second + 1});
            if (isLastSibling(c))
                break;
        }
    }
    return best;
}

uint32_t Dictionary::findExact(const std::u32string &word) const
{
    uint32_t n = 0;
    for (char32_t ch : word) {
        uint32_t c = firstChild(n);
        if (c == 0)
            return 0;
        for (;; ++c) {
            if (m_alphabet[m_nodes[c].ch].cp == static_cast<uint32_t>(ch))
                break;
            if (isLastSibling(c))
                return 0;
        }
        n = c;
    }
    return n != 0 && isTerminal(n) ? n : 0;
}

} // namespace parrotfish
