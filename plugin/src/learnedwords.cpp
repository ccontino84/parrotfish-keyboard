// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "learnedwords.h"

#include "languages.h"

#include <QVariantMap>

#include <algorithm>

namespace {

enum Roles { WordRole = Qt::UserRole + 1, LanguageRole, LanguageNameRole };

// A language's name, or its code if it isn't installed any more.
QString languageName(const QString &code)
{
    const QString name = Languages::instance().names({code});
    return name.isEmpty() ? code : name;
}

bool lessByName(const QString &a, const QString &b)
{
    return QString::localeAwareCompare(a.toLower(), b.toLower()) < 0;
}

} // namespace

LearnedWords::LearnedWords(QObject *parent)
    : QAbstractListModel(parent)
    , m_store(new LearningStore(LearningStore::defaultPath()))
{
    reload();
}

int LearnedWords::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant LearnedWords::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case WordRole: return r.word;
    case LanguageRole: return r.language;
    case LanguageNameRole: return r.languageName;
    default: return QVariant();
    }
}

QHash<int, QByteArray> LearnedWords::roleNames() const
{
    return {{WordRole, "word"}, {LanguageRole, "language"}, {LanguageNameRole, "languageName"}};
}

void LearnedWords::reload()
{
    beginResetModel();
    m_rows.clear();
    for (const LearningStore::ListedWord &w : m_store->listedWords(LearningStore::UndosToStop))
        m_rows.append({w.word, w.language, languageName(w.language)});
    std::sort(m_rows.begin(), m_rows.end(), [](const Row &a, const Row &b) {
        if (a.languageName != b.languageName)
            return lessByName(a.languageName, b.languageName);
        return lessByName(a.word, b.word);
    });
    endResetModel();
    emit countChanged();
}

void LearnedWords::forget(const QString &word, const QString &language)
{
    m_store->forgetWord(word, language);
    for (int row = m_rows.size() - 1; row >= 0; --row) {
        const Row &r = m_rows.at(row);
        if (r.word.toLower() != word.toLower() || (!language.isEmpty() && r.language != language))
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_rows.removeAt(row);
        endRemoveRows();
    }
    emit countChanged();
}

QVariantList LearnedWords::languages() const
{
    QList<QPair<QString, QString>> found;   // (name, code)
    for (const QString &code : m_store->languagesWithData())
        found.append(qMakePair(languageName(code), code));
    std::sort(found.begin(), found.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
        return lessByName(a.first, b.first);
    });
    QVariantList out;
    for (const auto &l : found)
        out.append(QVariantMap{{QStringLiteral("code"), l.second}, {QStringLiteral("name"), l.first}});
    return out;
}

void LearnedWords::clearLanguage(const QString &language)
{
    m_store->clearLanguage(language);
    reload();
}

void LearnedWords::clearAll()
{
    m_store->clearAll();
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit countChanged();
}
