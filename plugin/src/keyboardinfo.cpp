// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "keyboardinfo.h"
#include "learningstore.h"

#include <QDir>
#include <QFile>

KeyboardInfo::KeyboardInfo(QObject *parent)
    : QObject(parent)
{
}

QString KeyboardInfo::version() const
{
    return QStringLiteral(PARROTFISH_VERSION);
}

bool KeyboardInfo::debug() const
{
    return QFile::exists(QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/debug"));
}

QStringList KeyboardInfo::learningSummary() const
{
    return LearningStore(LearningStore::defaultPath()).summary();
}
