// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <string>
#include <vector>

#include "parrotfish/learned.h"

struct sqlite3;

// What the keyboard keeps between sessions, in one SQLite file, readable
// only by the user: the undo memory (typed words whose autocorrection the
// user undid, per language of the text they were typed in) and the learned
// words and word pairs (parrotfish::Learned). The engine
// keeps the data in memory; this file only stores it. Every table has a cap
// and drops what was used least recently, so the file doesn't grow without
// end. Without a file (it can't be opened), nothing is kept between sessions.
class LearningStore
{
public:
    explicit LearningStore(const QString &path);
    // ~/.local/share/parrotfish-keyboard/learning.db
    static QString defaultPath();
    ~LearningStore();
    LearningStore(const LearningStore &) = delete;
    LearningStore &operator=(const LearningStore &) = delete;

    bool isOpen() const { return m_db != nullptr; }

    // (language code, lowercase typed word)
    using UndoKey = QPair<QString, QString>;
    // → undos in a row.
    QHash<UndoKey, int> undos() const;
    // Stores the undos of `typed` (lowercase) in `language`; 0 forgets them.
    void setUndos(const QString &language, const QString &typed, int count);

    static const int MaxUndos = 1000;
    // Undos in a row after which a word isn't corrected again.
    static const int UndosToStop = 2;

    // The learned data (Learned's own caps keep it bounded).
    void loadLearned(parrotfish::Learned &learned) const;
    // What changed (Learned::takeChanges), copied out of the Learned: made
    // on the keyboard's thread, stored on the writer's (StoreWriter).
    struct LearnedRows {
        bool cleared = false;                // everything was forgotten first
        std::vector<std::string> rewrite;    // languages whose rows go before theirs are stored
        struct Word {
            std::string lang;
            std::u32string key;
            parrotfish::Learned::Word word;
            bool erase;
        };
        struct Pair {
            std::string lang;
            std::u32string prev, word;
            double count;   // 0: erased
        };
        struct Lang {
            std::string lang;
            double typed, nextFade;
        };
        std::vector<Word> words;
        std::vector<Pair> pairs;
        std::vector<Lang> langs;
        bool empty() const { return !cleared && rewrite.empty() && words.empty() && pairs.empty() && langs.empty(); }
    };
    static LearnedRows rows(const parrotfish::Learned &learned, const parrotfish::Learned::Changes &changes);
    // Stores them, in one transaction.
    void saveLearned(const LearnedRows &rows);

    // For the settings page (another process; the keyboard reloads after a
    // change): the words learned as new and those whose correction was
    // undone at least `undosToStop` times, one per language and lowercase
    // word (a new word's spelling first), unsorted.
    struct ListedWord {
        QString language, word;
    };
    QList<ListedWord> listedWords(int undosToStop) const;
    // Forgets a word in `language` (empty: in every language): its count,
    // its pairs, its undos.
    void forgetWord(const QString &word, const QString &language = QString());
    // Forgets everything learned in one language, its undos included.
    void clearLanguage(const QString &language);
    // Forgets everything learned, the undo memory included.
    void clearAll();
    // The languages something is learned in (words, pairs, undos or words
    // typed), as codes.
    QStringList languagesWithData() const;
    // For the diagnostics (settings page): per language, counts only, no
    // words: "en: typed 1234, words 567 (new 8, pending 9), pairs 1011,
    // undos 12".
    QStringList summary() const;

private:
    bool exec(const char *sql);
    int userVersion() const;
    void prune();

    sqlite3 *m_db = nullptr;
    int m_writes = 0;   // since the last prune
};
