// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "voicemodel.h"

#include "voicepaths.h"

#include <QDir>
#include <QDirIterator>

VoiceModel::VoiceModel(QObject *parent)
    : QObject(parent)
{
}

VoiceModel::~VoiceModel()
{
    cancel();
}

bool VoiceModel::installed() const
{
    return QFileInfo(VoicePaths::downloader()).isExecutable();
}

bool VoiceModel::ready() const
{
    return VoicePaths::modelReady();
}

qint64 VoiceModel::diskUsage() const
{
    qint64 sum = 0;
    QDirIterator it(VoicePaths::modelDir(), QDir::Files);
    while (it.hasNext()) {
        it.next();
        sum += it.fileInfo().size();
    }
    return sum;
}

void VoiceModel::download()
{
    if (m_process || !installed())
        return;
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &VoiceModel::readLines);
    connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
                m_process->deleteLater();
                m_process = nullptr;
                if (m_state != QStringLiteral("failed")) {
                    if (status != QProcess::NormalExit || code != 0) {
                        m_state = QStringLiteral("failed");
                        m_error = QStringLiteral("stopped");
                    } else {
                        m_state.clear();
                    }
                }
                emit changed();
            });
    m_buffer.clear();
    m_state = QStringLiteral("downloading");
    m_error.clear();
    emit changed();
    m_process->start(VoicePaths::downloader(), QStringList() << VoicePaths::modelDir());
}

void VoiceModel::cancel()
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

void VoiceModel::remove()
{
    cancel();
    QDir(VoicePaths::modelDir()).removeRecursively();
    emit changed();
}

void VoiceModel::readLines()
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
