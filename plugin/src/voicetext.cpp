// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "voicetext.h"

#include <algorithm>

namespace {

const QString Ending = QStringLiteral(".,;:!?…");

QString stripEnding(const QString &word, QString *ending = nullptr)
{
    int n = word.size();
    while (n > 0 && Ending.contains(word.at(n - 1)))
        --n;
    if (ending)
        *ending = word.mid(n);
    return word.left(n);
}

QStringList split(const QString &text)
{
    return text.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

} // namespace

void VoiceText::clear()
{
    m_words.clear();
    m_edits.clear();
    m_open.clear();
    m_selFrom = m_selTo = -1;
    m_replaceIndex = -1;
}

void VoiceText::startRecording()
{
    m_open.clear();
    m_openIndex = -1;
    m_dropUpTo = -1;
    m_replaceIndex = clampSelection() ? 0 : -1;
}

void VoiceText::fixed(int phrase, const QString &text)
{
    if (phrase > m_dropUpTo)
        commit(phrase, text);
    if (m_openIndex <= phrase)
        m_open.clear();
}

void VoiceText::open(int phrase, const QString &text)
{
    if (phrase <= m_dropUpTo)
        return;
    if (phrase == m_replaceIndex && clampSelection()) {
        // The replacement, said after a pause: final now, so that the
        // selection ends with it. Its later decodings are ignored.
        commit(phrase, text);
        m_dropUpTo = phrase;
        return;
    }
    m_open = text;
    m_openIndex = phrase;
}

void VoiceText::commitOpen()
{
    if (!m_open.isEmpty())
        commit(m_openIndex, m_open);
    m_open.clear();
}

bool VoiceText::undo()
{
    if (!m_open.isEmpty()) {
        // Its phrase may still come as fixed, decoded again with the next.
        m_dropUpTo = std::max(m_dropUpTo, m_openIndex);
        if (m_replaceIndex == m_openIndex)
            m_replaceIndex = m_openIndex + 1;
        m_open.clear();
        return true;
    }
    if (m_edits.isEmpty())
        return false;
    const Edit edit = m_edits.takeLast();
    m_words.erase(m_words.begin() + edit.at, m_words.begin() + edit.at + edit.inserted);
    for (int i = 0; i < edit.removed.size(); ++i)
        m_words.insert(edit.at + i, edit.removed.at(i));
    m_selFrom = m_selTo = -1;
    m_replaceIndex = -1;
    return true;
}

void VoiceText::select(int from, int to)
{
    m_selFrom = std::max(0, std::min(from, to));
    m_selTo = std::max(from, to);
}

bool VoiceText::clearSelection()
{
    if (m_selFrom < 0)
        return false;
    m_selFrom = m_selTo = -1;
    m_replaceIndex = -1;
    return true;
}

bool VoiceText::deleteSelection()
{
    if (!clampSelection())
        return false;
    replace(m_selFrom, m_selTo - m_selFrom + 1, QStringList());
    m_selFrom = m_selTo = -1;
    m_replaceIndex = -1;
    return true;
}

QString VoiceText::text() const
{
    const QString fixed = fixedText();
    if (fixed.isEmpty() || m_open.isEmpty())
        return fixed + m_open;
    return fixed + QLatin1Char(' ') + m_open;
}

QStringList VoiceText::fitReplacement(QStringList words, const QStringList &old)
{
    if (words.isEmpty() || old.isEmpty())
        return words;
    QString ending;
    stripEnding(old.last(), &ending);
    words.last() = stripEnding(words.last()) + ending;
    if (words.last().isEmpty())
        words.removeLast();
    if (words.isEmpty())
        return words;
    // A proper noun replacing a lowercase word loses its capital.
    QString &first = words.first();
    const QChar was = old.first().isEmpty() ? QChar() : old.first().at(0);
    if (was.isUpper()) {
        first[0] = first.at(0).toUpper();
    } else if (was.isLower() && first.size() > 1 && first.at(0).isUpper()
               && first.mid(1) == first.mid(1).toLower() && !first.startsWith(QStringLiteral("I'"))) {
        first[0] = first.at(0).toLower();
    }
    return words;
}

void VoiceText::commit(int phrase, const QString &text)
{
    QStringList words = split(text);
    if (words.isEmpty())
        return;
    if (phrase == m_replaceIndex && clampSelection()) {
        const int count = m_selTo - m_selFrom + 1;
        replace(m_selFrom, count, fitReplacement(words, m_words.mid(m_selFrom, count)));
        m_selFrom = m_selTo = -1;
        m_replaceIndex = -1;
        return;
    }
    replace(m_words.size(), 0, words);
}

// Words at..at+count become `words`, as an edit that undo() takes back.
void VoiceText::replace(int at, int count, const QStringList &words)
{
    m_edits.append(Edit{at, m_words.mid(at, count), words.size()});
    m_words.erase(m_words.begin() + at, m_words.begin() + at + count);
    for (int i = 0; i < words.size(); ++i)
        m_words.insert(at + i, words.at(i));
}

// The selection was made on the words shown, which may have included the
// open phrase, fixed since.
bool VoiceText::clampSelection()
{
    if (m_selFrom >= m_words.size())
        m_selFrom = m_selTo = -1;
    else if (m_selTo >= m_words.size())
        m_selTo = m_words.size() - 1;
    return m_selFrom >= 0;
}
