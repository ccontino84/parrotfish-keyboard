// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "downloadjob.h"

#include <QObject>
#include <QStringList>
#include <QVariantMap>

// Dictionary downloads for the settings (QML singleton LanguageDownloads),
// one at a time through DownloadJob. After each download or delete it
// updates Languages, switches a new language on (off when deleted with no
// package left) and tells the keyboard (dictionaries-changed).
class LanguageDownloads : public QObject
{
    Q_OBJECT
    // CODE: {state: "queued", "downloading", "verifying" or "failed",
    // error (DownloadJob::error), received, total}. Idle ones are absent.
    Q_PROPERTY(QVariantMap jobs READ jobs NOTIFY changed)

public:
    explicit LanguageDownloads(QObject *parent = nullptr);

    QVariantMap jobs() const;

    Q_INVOKABLE void download(const QString &code);
    // Every installed language with an update.
    Q_INVOKABLE void updateAll();
    // Stops or unqueues it; a failed one is forgotten.
    Q_INVOKABLE void cancel(const QString &code);
    // Deletes the downloaded dictionary (the package's stays).
    Q_INVOKABLE void remove(const QString &code);

signals:
    void changed();

private:
    void next();
    void done(bool ok);
    // Installed before the download: the languages typed with.
    void changedFiles(const QString &code, bool wasInstalled, const QStringList &typed);
    QStringList typedLanguages() const;

    DownloadJob m_job;
    QString m_current;
    QStringList m_queue;
    QVariantMap m_failed;   // code: error
};
