// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "voicemodel.h"

#include "voicepaths.h"

#include <QDir>
#include <QDirIterator>

VoiceModel::VoiceModel(QObject *parent)
    : QObject(parent)
{
    connect(&m_job, &DownloadJob::changed, this, &VoiceModel::changed);
}

VoiceModel::~VoiceModel() = default;

bool VoiceModel::installed() const
{
    return QFileInfo(VoicePaths::helper()).isExecutable();
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
    if (installed())
        m_job.start({QStringLiteral("voice"), VoicePaths::modelDir()});
}

void VoiceModel::cancel()
{
    m_job.cancel();
}

void VoiceModel::remove()
{
    m_job.cancel();
    QDir(VoicePaths::modelDir()).removeRecursively();
    emit changed();
}
