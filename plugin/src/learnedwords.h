// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "learningstore.h"

#include <QAbstractListModel>
#include <QStringList>

#include <memory>

// The learned words for the settings page (Settings → Text input →
// Learned words), from learning.db: the words learned as new and those
// whose correction the user undid, one row each (role "word"). The
// settings page runs in another process than the keyboard: after forget()
// or clearAll() it tells the keyboard to reload (a dconf key).
class LearnedWords : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LearnedWords(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_words.size(); }

    // Reads the list again.
    Q_INVOKABLE void reload();
    // Forgets `word` in every language: its count, pairs and undos.
    Q_INVOKABLE void forget(const QString &word);
    // Forgets everything learned, the undo memory included.
    Q_INVOKABLE void clearAll();

signals:
    void countChanged();

private:
    std::unique_ptr<LearningStore> m_store;
    QStringList m_words;
};
