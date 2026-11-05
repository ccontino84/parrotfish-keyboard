// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "learningstore.h"

#include <QDateTime>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QtDebug>

#include <sqlite3.h>

#include "parrotfish/utf8.h"

namespace {

// Prune after this many writes (and at every start).
const int PruneEvery = 100;

} // namespace

QString LearningStore::defaultPath()
{
    return QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/learning.db");
}

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
    // A write waits for another's: the keyboard's writer storing a language
    // whole after fading (a second or so on a phone), the settings page.
    sqlite3_busy_timeout(m_db, 5000);
    // auto_vacuum only takes effect before the first table is created: freed
    // pages are given back by prune().
    if (!exec("PRAGMA auto_vacuum = INCREMENTAL")
            || !exec("PRAGMA journal_mode = WAL")
            || !exec("PRAGMA synchronous = NORMAL")
            // Version 1 kept undos without their language: they are dropped.
            || (userVersion() < 2 && !exec("DROP TABLE IF EXISTS undos"))
            || !exec("CREATE TABLE IF NOT EXISTS undos ("
                     " lang TEXT NOT NULL,"
                     " typed TEXT NOT NULL,"
                     " count INTEGER NOT NULL,"
                     " used INTEGER NOT NULL,"
                     " PRIMARY KEY (lang, typed))")
            // Version 3: the learned words, word pairs and per-language
            // counters (parrotfish::Learned).
            || !exec("CREATE TABLE IF NOT EXISTS words ("
                     " lang TEXT NOT NULL,"
                     " key TEXT NOT NULL,"
                     " spelling TEXT NOT NULL,"
                     " count REAL NOT NULL,"
                     " is_new INTEGER NOT NULL,"
                     " pending INTEGER NOT NULL,"
                     " texts INTEGER NOT NULL,"
                     " first_seen REAL NOT NULL,"
                     " last_used REAL NOT NULL DEFAULT 0,"
                     " PRIMARY KEY (lang, key))")
            // Version 4: when each word was last kept (trimming).
            || (userVersion() == 3 && !exec("ALTER TABLE words ADD COLUMN last_used REAL NOT NULL DEFAULT 0"))
            || !exec("CREATE TABLE IF NOT EXISTS pairs ("
                     " lang TEXT NOT NULL,"
                     " prev TEXT NOT NULL,"
                     " word TEXT NOT NULL,"
                     " count REAL NOT NULL,"
                     " PRIMARY KEY (lang, prev, word))")
            || !exec("CREATE TABLE IF NOT EXISTS langs ("
                     " lang TEXT PRIMARY KEY,"
                     " typed REAL NOT NULL,"
                     " next_fade REAL NOT NULL)")
            || !exec("PRAGMA user_version = 4")) {
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }
    // What the user typed: theirs only. The folder too (slips.log), and the
    // WAL files, which keep their mode while the file stays open (a newly
    // created one takes the database's).
    const QFileDevice::Permissions owner = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    QFile::setPermissions(QFileInfo(path).absolutePath(), owner | QFileDevice::ExeOwner);
    for (const QString &file : {path, path + QStringLiteral("-wal"), path + QStringLiteral("-shm")})
        if (QFile::exists(file))
            QFile::setPermissions(file, owner);
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

int LearningStore::userVersion() const
{
    sqlite3_stmt *st = nullptr;
    int version = 0;
    if (sqlite3_prepare_v2(m_db, "PRAGMA user_version", -1, &st, nullptr) == SQLITE_OK
            && sqlite3_step(st) == SQLITE_ROW)
        version = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return version;
}

QHash<LearningStore::UndoKey, int> LearningStore::undos() const
{
    QHash<UndoKey, int> out;
    sqlite3_stmt *st = nullptr;
    if (!m_db || sqlite3_prepare_v2(m_db, "SELECT lang, typed, count FROM undos", -1, &st, nullptr) != SQLITE_OK)
        return out;
    auto text = [st](int column) {
        return QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, column)));
    };
    while (sqlite3_step(st) == SQLITE_ROW)
        out.insert(qMakePair(text(0), text(1)), sqlite3_column_int(st, 2));
    sqlite3_finalize(st);
    return out;
}

void LearningStore::setUndos(const QString &language, const QString &typed, int count)
{
    if (!m_db)
        return;
    sqlite3_stmt *st = nullptr;
    const char *sql = count > 0
            ? "INSERT OR REPLACE INTO undos (lang, typed, count, used) VALUES (?1, ?2, ?3, ?4)"
            : "DELETE FROM undos WHERE lang = ?1 AND typed = ?2";
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) != SQLITE_OK)
        return;
    const QByteArray l = language.toUtf8(), t = typed.toUtf8();
    sqlite3_bind_text(st, 1, l.constData(), l.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, t.constData(), t.size(), SQLITE_TRANSIENT);
    if (count > 0) {
        sqlite3_bind_int(st, 3, count);
        sqlite3_bind_int64(st, 4, QDateTime::currentMSecsSinceEpoch() / 1000);
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
    const QByteArray sql = QByteArray("DELETE FROM undos WHERE rowid NOT IN"
                                      " (SELECT rowid FROM undos ORDER BY used DESC, rowid DESC LIMIT ")
            + QByteArray::number(MaxUndos) + ")";
    exec(sql.constData());
    exec("PRAGMA incremental_vacuum");
}

namespace {

QByteArray utf8(const std::u32string &s)
{
    return QByteArray::fromStdString(parrotfish::utf8::encode(s));
}

std::u32string u32(sqlite3_stmt *st, int column)
{
    return parrotfish::utf8::decode(reinterpret_cast<const char *>(sqlite3_column_text(st, column)));
}

// A prepared statement, finalized on scope exit.
struct Statement {
    sqlite3_stmt *st = nullptr;
    Statement(sqlite3 *db, const char *sql) { sqlite3_prepare_v2(db, sql, -1, &st, nullptr); }
    ~Statement() { sqlite3_finalize(st); }
    explicit operator bool() const { return st != nullptr; }
    void text(int i, const QByteArray &v) { sqlite3_bind_text(st, i, v.constData(), v.size(), SQLITE_TRANSIENT); }
    void real(int i, double v) { sqlite3_bind_double(st, i, v); }
    void integer(int i, int v) { sqlite3_bind_int(st, i, v); }
    bool run()
    {
        const bool ok = sqlite3_step(st) == SQLITE_DONE;
        sqlite3_reset(st);
        sqlite3_clear_bindings(st);
        return ok;
    }
};

} // namespace

void LearningStore::loadLearned(parrotfish::Learned &learned) const
{
    if (!m_db)
        return;
    sqlite3_stmt *st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT lang, typed, next_fade FROM langs", -1, &st, nullptr) == SQLITE_OK)
        while (sqlite3_step(st) == SQLITE_ROW)
            learned.restoreLanguage(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)),
                                    sqlite3_column_double(st, 1), sqlite3_column_double(st, 2));
    sqlite3_finalize(st);
    st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT lang, key, spelling, count, is_new, pending, texts, first_seen, last_used"
                                 " FROM words", -1, &st, nullptr) == SQLITE_OK) {
        while (sqlite3_step(st) == SQLITE_ROW) {
            parrotfish::Learned::Word w;
            w.spelling = u32(st, 2);
            w.count = sqlite3_column_double(st, 3);
            w.isNew = sqlite3_column_int(st, 4) != 0;
            w.pending = sqlite3_column_int(st, 5) != 0;
            w.texts = sqlite3_column_int(st, 6);
            w.firstSeen = sqlite3_column_double(st, 7);
            w.lastUsed = sqlite3_column_double(st, 8);
            learned.restoreWord(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)), u32(st, 1), w);
        }
    }
    sqlite3_finalize(st);
    st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT lang, prev, word, count FROM pairs", -1, &st, nullptr) == SQLITE_OK)
        while (sqlite3_step(st) == SQLITE_ROW)
            learned.restorePair(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)), u32(st, 1), u32(st, 2),
                                sqlite3_column_double(st, 3));
    sqlite3_finalize(st);
    learned.finishRestore();
}

LearningStore::LearnedRows LearningStore::rows(const parrotfish::Learned &learned,
                                               const parrotfish::Learned::Changes &changes)
{
    using Learned = parrotfish::Learned;
    LearnedRows out;
    out.cleared = changes.cleared;
    // Languages stored whole (faded or trimmed: every count changed).
    for (const std::string &code : changes.rewrite) {
        out.rewrite.push_back(code);
        const Learned::Language *l = learned.language(code);
        if (!l)
            continue;
        for (const auto &w : l->words)
            out.words.push_back({code, w.first, w.second, false});
        for (const auto &p : l->next)
            for (const auto &n : p.second)
                out.pairs.push_back({code, p.first, n.first, n.second});
    }
    for (const auto &key : changes.words) {
        if (changes.rewrite.count(key.first))
            continue;
        const Learned::Language *l = learned.language(key.first);
        const auto it = l ? l->words.find(key.second) : decltype(l->words.end())();
        if (l && it != l->words.end())
            out.words.push_back({key.first, key.second, it->second, false});
        else
            out.words.push_back({key.first, key.second, Learned::Word(), true});
    }
    for (const auto &key : changes.pairs) {
        const std::string &code = std::get<0>(key);
        if (changes.rewrite.count(code))
            continue;
        const Learned::Language *l = learned.language(code);
        double count = 0;
        if (l) {
            const auto p = l->next.find(std::get<1>(key));
            if (p != l->next.end()) {
                const auto n = p->second.find(std::get<2>(key));
                if (n != p->second.end())
                    count = n->second;
            }
        }
        out.pairs.push_back({code, std::get<1>(key), std::get<2>(key), count});
    }
    std::set<std::string> languages = changes.languages;
    languages.insert(changes.rewrite.begin(), changes.rewrite.end());
    for (const std::string &code : languages)
        if (const Learned::Language *l = learned.language(code))
            out.langs.push_back({code, l->typed, l->nextFade});
    return out;
}

void LearningStore::saveLearned(const LearnedRows &rows)
{
    if (!m_db || rows.empty())
        return;
    if (!exec("BEGIN"))
        return;
    if (rows.cleared) {
        exec("DELETE FROM words");
        exec("DELETE FROM pairs");
        exec("DELETE FROM langs");
    }
    Statement putWord(m_db, "INSERT OR REPLACE INTO words (lang, key, spelling, count, is_new, pending, texts,"
                            " first_seen, last_used) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)");
    Statement delWord(m_db, "DELETE FROM words WHERE lang = ?1 AND key = ?2");
    Statement putPair(m_db, "INSERT OR REPLACE INTO pairs (lang, prev, word, count) VALUES (?1, ?2, ?3, ?4)");
    Statement delPair(m_db, "DELETE FROM pairs WHERE lang = ?1 AND prev = ?2 AND word = ?3");
    Statement putLang(m_db, "INSERT OR REPLACE INTO langs (lang, typed, next_fade) VALUES (?1, ?2, ?3)");
    Statement delLangWords(m_db, "DELETE FROM words WHERE lang = ?1");
    Statement delLangPairs(m_db, "DELETE FROM pairs WHERE lang = ?1");
    if (!putWord || !delWord || !putPair || !delPair || !putLang || !delLangWords || !delLangPairs) {
        exec("ROLLBACK");
        return;
    }
    for (const std::string &code : rows.rewrite) {
        const QByteArray lang = QByteArray::fromStdString(code);
        delLangWords.text(1, lang);
        delLangWords.run();
        delLangPairs.text(1, lang);
        delLangPairs.run();
    }
    for (const LearnedRows::Word &r : rows.words) {
        const QByteArray lang = QByteArray::fromStdString(r.lang);
        if (r.erase) {
            delWord.text(1, lang);
            delWord.text(2, utf8(r.key));
            delWord.run();
            continue;
        }
        putWord.text(1, lang);
        putWord.text(2, utf8(r.key));
        putWord.text(3, utf8(r.word.spelling));
        putWord.real(4, r.word.count);
        putWord.integer(5, r.word.isNew);
        putWord.integer(6, r.word.pending);
        putWord.integer(7, r.word.texts);
        putWord.real(8, r.word.firstSeen);
        putWord.real(9, r.word.lastUsed);
        putWord.run();
    }
    for (const LearnedRows::Pair &r : rows.pairs) {
        Statement &st = r.count > 0 ? putPair : delPair;
        st.text(1, QByteArray::fromStdString(r.lang));
        st.text(2, utf8(r.prev));
        st.text(3, utf8(r.word));
        if (r.count > 0)
            st.real(4, r.count);
        st.run();
    }
    for (const LearnedRows::Lang &r : rows.langs) {
        putLang.text(1, QByteArray::fromStdString(r.lang));
        putLang.real(2, r.typed);
        putLang.real(3, r.nextFade);
        putLang.run();
    }
    if (!exec("COMMIT"))
        exec("ROLLBACK");
}

QList<LearningStore::ListedWord> LearningStore::listedWords(int undosToStop) const
{
    // (language, lowercase) → spelling, a new word's first
    QMap<QPair<QString, QString>, QString> byKey;
    if (!m_db)
        return {};
    auto column = [](sqlite3_stmt *st, int i) {
        return QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, i)));
    };
    sqlite3_stmt *st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT lang, key, spelling FROM words WHERE is_new = 1 AND pending = 0", -1,
                           &st, nullptr) == SQLITE_OK)
        while (sqlite3_step(st) == SQLITE_ROW)
            byKey.insert(qMakePair(column(st, 0), column(st, 1)), column(st, 2));
    sqlite3_finalize(st);
    st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT lang, typed FROM undos WHERE count >= ?1", -1, &st, nullptr)
            == SQLITE_OK) {
        sqlite3_bind_int(st, 1, undosToStop);
        while (sqlite3_step(st) == SQLITE_ROW) {
            const QPair<QString, QString> key(column(st, 0), column(st, 1));
            if (!byKey.contains(key))
                byKey.insert(key, key.second);
        }
    }
    sqlite3_finalize(st);
    QList<ListedWord> out;
    for (auto it = byKey.constBegin(); it != byKey.constEnd(); ++it)
        out.append({it.key().first, it.value()});
    return out;
}

void LearningStore::forgetWord(const QString &word, const QString &language)
{
    if (!m_db)
        return;
    const QByteArray key = word.toLower().toUtf8();
    const QByteArray lang = language.toUtf8();
    if (!exec("BEGIN"))
        return;
    // ?2 = '' matches every language
    for (const char *sql : {"DELETE FROM words WHERE key = ?1 AND (?2 = '' OR lang = ?2)",
                            "DELETE FROM pairs WHERE (prev = ?1 OR word = ?1) AND (?2 = '' OR lang = ?2)",
                            "DELETE FROM undos WHERE typed = ?1 AND (?2 = '' OR lang = ?2)"}) {
        Statement st(m_db, sql);
        if (!st)
            continue;
        st.text(1, key);
        st.text(2, lang);
        st.run();
    }
    if (!exec("COMMIT"))
        exec("ROLLBACK");
}

void LearningStore::clearLanguage(const QString &language)
{
    if (!m_db || language.isEmpty())
        return;
    const QByteArray lang = language.toUtf8();
    if (!exec("BEGIN"))
        return;
    for (const char *sql : {"DELETE FROM words WHERE lang = ?1", "DELETE FROM pairs WHERE lang = ?1",
                            "DELETE FROM langs WHERE lang = ?1", "DELETE FROM undos WHERE lang = ?1"}) {
        Statement st(m_db, sql);
        if (!st)
            continue;
        st.text(1, lang);
        st.run();
    }
    if (!exec("COMMIT"))
        exec("ROLLBACK");
    exec("PRAGMA incremental_vacuum");
}

void LearningStore::clearAll()
{
    if (!m_db)
        return;
    exec("BEGIN");
    exec("DELETE FROM words");
    exec("DELETE FROM pairs");
    exec("DELETE FROM langs");
    exec("DELETE FROM undos");
    if (!exec("COMMIT"))
        exec("ROLLBACK");
    exec("PRAGMA incremental_vacuum");
}

QStringList LearningStore::languagesWithData() const
{
    QStringList out;
    if (!m_db)
        return out;
    sqlite3_stmt *st = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "SELECT lang FROM words UNION SELECT lang FROM pairs UNION SELECT lang FROM undos"
                           " UNION SELECT lang FROM langs WHERE typed > 0",
                           -1, &st, nullptr) == SQLITE_OK)
        while (sqlite3_step(st) == SQLITE_ROW)
            out << QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)));
    sqlite3_finalize(st);
    return out;
}

QStringList LearningStore::summary() const
{
    if (!m_db)
        return {QStringLiteral("no learning.db")};
    struct Counts {
        qint64 typed = 0, words = 0, isNew = 0, pending = 0, pairs = 0, undos = 0;
    };
    QMap<QString, Counts> byLang;
    // (sql, the field each row's second column goes to)
    const auto query = [&](const char *sql, qint64 Counts::*field) {
        sqlite3_stmt *st = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) == SQLITE_OK)
            while (sqlite3_step(st) == SQLITE_ROW)
                byLang[QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(st, 0)))].*field
                        = sqlite3_column_int64(st, 1);
        sqlite3_finalize(st);
    };
    query("SELECT lang, CAST(typed AS INTEGER) FROM langs", &Counts::typed);
    query("SELECT lang, COUNT(*) FROM words WHERE pending = 0 GROUP BY lang", &Counts::words);
    query("SELECT lang, COUNT(*) FROM words WHERE is_new = 1 AND pending = 0 GROUP BY lang", &Counts::isNew);
    query("SELECT lang, COUNT(*) FROM words WHERE pending = 1 GROUP BY lang", &Counts::pending);
    query("SELECT lang, COUNT(*) FROM pairs GROUP BY lang", &Counts::pairs);
    query("SELECT lang, COUNT(*) FROM undos GROUP BY lang", &Counts::undos);
    QStringList out;
    for (auto it = byLang.constBegin(); it != byLang.constEnd(); ++it) {
        const Counts &c = it.value();
        out << QStringLiteral("%1: typed %2, words %3 (new %4, pending %5), pairs %6, undos %7")
                   .arg(it.key()).arg(c.typed).arg(c.words).arg(c.isNew).arg(c.pending).arg(c.pairs).arg(c.undos);
    }
    if (out.isEmpty())
        out << QStringLiteral("nothing learned");
    return out;
}
