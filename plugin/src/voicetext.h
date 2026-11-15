// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// The dictated text while voice input listens (VoiceInput): the fixed
// words, the open phrase (which can still change), and the user's edits:
// undo, a selection of words, its deletion or its replacement by the next
// phrase. Phrases come numbered from the helper, from 0 in each recording.
class VoiceText
{
public:
    void clear();
    // A recording starts: with a selection, its first phrase replaces it.
    void startRecording();

    // The helper's "fixed" and "open" lines.
    void fixed(int phrase, const QString &text);
    void open(int phrase, const QString &text);
    // The recording ended: the open phrase becomes fixed.
    void commitOpen();

    // Removes the open phrase if any, else takes the last edit back.
    bool undo();
    void select(int from, int to);
    bool clearSelection();
    bool deleteSelection();

    QString text() const;
    QString fixedText() const { return m_words.join(QLatin1Char(' ')); }
    QStringList words() const { return m_words; }
    QString openText() const { return m_open; }
    int selectionStart() const { return m_selFrom; }
    int selectionEnd() const { return m_selTo; }
    bool replacing() const { return m_replaceIndex >= 0 && m_selFrom >= 0; }
    // What undo() changes when there is no open phrase: it removes words
    // undoFrom to undoTo (exclusive) and puts undoRestores there.
    int undoFrom() const { return m_edits.isEmpty() ? -1 : m_edits.last().at; }
    int undoTo() const { return m_edits.isEmpty() ? -1 : m_edits.last().at + m_edits.last().inserted; }
    QStringList undoRestores() const { return m_edits.isEmpty() ? QStringList() : m_edits.last().removed; }

    // A replacement said on its own comes as a sentence ("Three o'clock."):
    // it takes the case and the final punctuation of the words it replaces.
    static QStringList fitReplacement(QStringList words, const QStringList &old);

private:
    struct Edit {
        int at;
        QStringList removed;
        int inserted;
    };

    void commit(int phrase, const QString &text);
    void replace(int at, int count, const QStringList &words);
    bool clampSelection();

    QStringList m_words;
    QVector<Edit> m_edits;
    QString m_open;
    int m_openIndex = -1;
    int m_dropUpTo = -1;     // phrases up to this number were undone or used
    int m_selFrom = -1;      // the selected words, inclusive
    int m_selTo = -1;
    int m_replaceIndex = -1; // the phrase that replaces the selection
};
