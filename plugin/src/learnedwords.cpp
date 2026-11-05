// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "learnedwords.h"

#include <QDir>

LearnedWords::LearnedWords(QObject *parent)
    : QAbstractListModel(parent)
    , m_store(new LearningStore(LearningStore::defaultPath()))
{
    reload();
}

int LearnedWords::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_words.size();
}

QVariant LearnedWords::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_words.size() || role != Qt::UserRole + 1)
        return QVariant();
    return m_words.at(index.row());
}

QHash<int, QByteArray> LearnedWords::roleNames() const
{
    return {{Qt::UserRole + 1, "word"}};
}

void LearnedWords::reload()
{
    beginResetModel();
    m_words = m_store->listedWords(LearningStore::UndosToStop);
    endResetModel();
    emit countChanged();
}

void LearnedWords::forget(const QString &word)
{
    m_store->forgetWord(word);
    const int row = m_words.indexOf(word);
    if (row < 0)
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_words.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void LearnedWords::clearAll()
{
    m_store->clearAll();
    beginResetModel();
    m_words.clear();
    endResetModel();
    emit countChanged();
}
