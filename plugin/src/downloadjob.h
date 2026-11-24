// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

// A run of parrotfish-download (downloader/), which does the network part
// of the settings' downloads: its progress and how it ended.
class DownloadJob : public QObject
{
    Q_OBJECT

public:
    explicit DownloadJob(QObject *parent = nullptr);
    ~DownloadJob() override;

    static QString program();

    void start(const QStringList &arguments);
    // Stops it; what was received is kept for the next run.
    void cancel();
    bool running() const { return m_process != nullptr; }

    // "" (idle), "downloading", "verifying" or "failed" (see error).
    QString state() const { return m_state; }
    // The downloader's error line: "space <bytes>", "network <text>",
    // "checksum <file>", "write <path>", "unavailable" or "stopped".
    QString error() const { return m_error; }
    qint64 received() const { return m_received; }
    qint64 total() const { return m_total; }

signals:
    void changed();
    void finished(bool ok);

private:
    void readLines();

    QProcess *m_process = nullptr;
    QByteArray m_buffer;
    QString m_state;
    QString m_error;
    qint64 m_received = 0;
    qint64 m_total = 0;
};
