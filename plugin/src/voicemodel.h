// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

// The speech model for the settings (QML singleton VoiceModel): whether it
// is downloaded, and its download through the voice package's downloader
// (parrotfish-voice-download), which does the network part.
class VoiceModel : public QObject
{
    Q_OBJECT
    // The voice package is installed.
    Q_PROPERTY(bool installed READ installed CONSTANT)
    // The model is downloaded and checked.
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    // "" (idle), "downloading", "verifying" or "failed" (see error).
    Q_PROPERTY(QString state READ state NOTIFY changed)
    // The downloader's error line: "space <bytes>", "network <text>",
    // "checksum <file>", "write <path>" or "stopped".
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(qint64 received READ received NOTIFY changed)
    Q_PROPERTY(qint64 total READ total NOTIFY changed)
    // Bytes on disk, the downloaded parts included.
    Q_PROPERTY(qint64 diskUsage READ diskUsage NOTIFY changed)

public:
    explicit VoiceModel(QObject *parent = nullptr);
    ~VoiceModel() override;

    bool installed() const;
    bool ready() const;
    QString state() const { return m_state; }
    QString error() const { return m_error; }
    qint64 received() const { return m_received; }
    qint64 total() const { return m_total; }
    qint64 diskUsage() const;

    Q_INVOKABLE void download();
    // Stops the download; what was received is kept for the next one.
    Q_INVOKABLE void cancel();
    // Deletes the model and any partial download.
    Q_INVOKABLE void remove();

signals:
    void changed();

private:
    void readLines();

    QProcess *m_process = nullptr;
    QByteArray m_buffer;
    QString m_state;
    QString m_error;
    qint64 m_received = 0;
    qint64 m_total = 0;
};
