// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "learningstore.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

#include <memory>

// The learned words for the settings page (Settings → Text input →
// Learned words), from learning.db: the words learned as new and those
// whose correction the user undid, one row per language and word (roles
// "word", "language": its code, "languageName"), sorted by language name,
// then word. The settings page runs in another process than the keyboard:
// after forget(), clearLanguage() or clearAll() it tells the keyboard to
// reload (a dconf key).
class LearnedWords : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LearnedWords(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_rows.size(); }

    // Reads the list again.
    Q_INVOKABLE void reload();
    // Forgets `word` in `language` (empty: in every language): its count,
    // pairs and undos.
    Q_INVOKABLE void forget(const QString &word, const QString &language = QString());
    // The languages something is learned in, also without listed words:
    // [{code, name}], sorted by name.
    Q_INVOKABLE QVariantList languages() const;
    // Forgets everything learned in one language.
    Q_INVOKABLE void clearLanguage(const QString &language);
    // Forgets everything learned, the undo memory included.
    Q_INVOKABLE void clearAll();

signals:
    void countChanged();

private:
    struct Row {
        QString word, language, languageName;
    };
    std::unique_ptr<LearningStore> m_store;
    QList<Row> m_rows;
};
