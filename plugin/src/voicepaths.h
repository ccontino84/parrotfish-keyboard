// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

// Where the voice helper is, and where the speech
// model is downloaded to (per user: the download needs no root).
namespace VoicePaths {

inline QString helper()
{
    return QStringLiteral(PARROTFISH_VOICE_LIBEXEC_DIR "/parrotfish-voice");
}

inline QString modelDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/parrotfish-keyboard/voice/parakeet-tdt-0.6b-ultra-int8");
}

// The downloader writes "revision" last, once every file is checked.
inline bool modelReady()
{
    return QFileInfo::exists(modelDir() + QStringLiteral("/revision"));
}

} // namespace VoicePaths
