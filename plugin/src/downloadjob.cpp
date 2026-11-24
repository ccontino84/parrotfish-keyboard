// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "downloadjob.h"

DownloadJob::DownloadJob(QObject *parent)
    : QObject(parent)
{
}

DownloadJob::~DownloadJob()
{
    cancel();
}

QString DownloadJob::program()
{
    return QStringLiteral(PARROTFISH_LIBEXEC_DIR "/parrotfish-download");
}

void DownloadJob::start(const QStringList &arguments)
{
    if (m_process)
        return;
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DownloadJob::readLines);
    connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
                m_process->deleteLater();
                m_process = nullptr;
                const bool ok = m_state != QStringLiteral("failed") && status == QProcess::NormalExit && code == 0;
                if (!ok && m_state != QStringLiteral("failed")) {
                    m_state = QStringLiteral("failed");
                    m_error = QStringLiteral("stopped");
                } else if (ok) {
                    m_state.clear();
                }
                emit changed();
                emit finished(ok);
            });
    m_buffer.clear();
    m_state = QStringLiteral("downloading");
    m_error.clear();
    m_received = 0;
    m_total = 0;
    emit changed();
    m_process->start(program(), arguments);
}

void DownloadJob::cancel()
{
    if (!m_process)
        return;
    QProcess *process = m_process;
    m_process = nullptr;
    process->disconnect(this);
    process->kill();
    process->waitForFinished(1000);
    process->deleteLater();
    m_state.clear();
    emit changed();
}

void DownloadJob::readLines()
{
    m_buffer += m_process->readAllStandardOutput();
    int end;
    while ((end = m_buffer.indexOf('\n')) >= 0) {
        const QStringList parts = QString::fromUtf8(m_buffer.left(end)).split(QLatin1Char(' '));
        m_buffer.remove(0, end + 1);
        const QString word = parts.value(0);
        if (word == QStringLiteral("progress") && parts.size() >= 3) {
            m_received = parts.at(1).toLongLong();
            m_total = parts.at(2).toLongLong();
        } else if (word == QStringLiteral("verifying")) {
            m_state = QStringLiteral("verifying");
        } else if (word == QStringLiteral("error")) {
            m_state = QStringLiteral("failed");
            m_error = QStringList(parts.mid(1)).join(QLatin1Char(' '));
        }
        emit changed();
    }
}
