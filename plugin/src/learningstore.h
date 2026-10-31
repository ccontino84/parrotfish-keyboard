// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <QHash>
#include <QPair>
#include <QString>

struct sqlite3;

// What the keyboard keeps between sessions, in one SQLite file. Today: the
// undo memory (typed words whose autocorrection the user undid, per language
// of the text they were typed in). The engine
// keeps the data in memory; this file only stores it. Every table has a cap
// and drops what was used least recently, so the file doesn't grow without
// end. Without a file (it can't be opened), nothing is kept between sessions.
class LearningStore
{
public:
    explicit LearningStore(const QString &path);
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

private:
    bool exec(const char *sql);
    int userVersion() const;
    void prune();

    sqlite3 *m_db = nullptr;
    int m_writes = 0;   // since the last prune
};
