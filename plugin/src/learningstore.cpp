// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "learningstore.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QtDebug>

#include <sqlite3.h>

namespace {

// Prune after this many writes (and at every start).
const int PruneEvery = 100;

} // namespace

LearningStore::LearningStore(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (sqlite3_open_v2(path.toUtf8().constData(), &m_db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        qWarning("parrotfish: can't open %s: %s", qPrintable(path), sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }
    sqlite3_busy_timeout(m_db, 200);
    // auto_vacuum only takes effect before the first table is created: freed
    // pages are given back by prune().
    if (!exec("PRAGMA auto_vacuum = INCREMENTAL")
            || !exec("PRAGMA journal_mode = WAL")
            || !exec("PRAGMA synchronous = NORMAL")
            || !exec("CREATE TABLE IF NOT EXISTS undos ("
                     " typed TEXT PRIMARY KEY,"
                     " count INTEGER NOT NULL,"
                     " used INTEGER NOT NULL)")
            || !exec("PRAGMA user_version = 1")) {
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }
    prune();
}

LearningStore::~LearningStore()
{
    if (m_db)
        sqlite3_close(m_db);
}

bool LearningStore::exec(const char *sql)
{
    char *error = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, nullptr, &error) == SQLITE_OK)
        return true;
    qWarning("parrotfish: learning store: %s", error ? error : "error");
    sqlite3_free(error);
    return false;
}

QHash<QString, int> LearningStore::undos() const
{
    QHash<QString, int> out;
    sqlite3_stmt *st = nullptr;
    if (!m_db || sqlite3_prepare_v2(m_db, "SELECT typed, count FROM undos", -1, &st, nullptr) != SQLITE_OK)
        return out;
    while (sqlite3_step(st) == SQLITE_ROW)
        out.insert(QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, 0))),
                   sqlite3_column_int(st, 1));
    sqlite3_finalize(st);
    return out;
}

void LearningStore::setUndos(const QString &typed, int count)
{
    if (!m_db)
        return;
    sqlite3_stmt *st = nullptr;
    const char *sql = count > 0
            ? "INSERT OR REPLACE INTO undos (typed, count, used) VALUES (?1, ?2, ?3)"
            : "DELETE FROM undos WHERE typed = ?1";
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) != SQLITE_OK)
        return;
    const QByteArray t = typed.toUtf8();
    sqlite3_bind_text(st, 1, t.constData(), t.size(), SQLITE_TRANSIENT);
    if (count > 0) {
        sqlite3_bind_int(st, 2, count);
        sqlite3_bind_int64(st, 3, QDateTime::currentMSecsSinceEpoch() / 1000);
    }
    if (sqlite3_step(st) != SQLITE_DONE)
        qWarning("parrotfish: learning store: %s", sqlite3_errmsg(m_db));
    sqlite3_finalize(st);
    if (++m_writes >= PruneEvery)
        prune();
}

// Keeps the MaxUndos most recently used undos and gives freed space back.
// Within a second, the later write wins: REPLACE gives the row a new rowid.
void LearningStore::prune()
{
    m_writes = 0;
    const QByteArray sql = QByteArray("DELETE FROM undos WHERE typed NOT IN"
                                      " (SELECT typed FROM undos ORDER BY used DESC, rowid DESC LIMIT ")
            + QByteArray::number(MaxUndos) + ")";
    exec(sql.constData());
    exec("PRAGMA incremental_vacuum");
}
