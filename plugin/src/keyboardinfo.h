// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <QObject>
#include <QStringList>

// For the settings pages: the installed version, the diagnostics a bug
// report needs, and setting everything back to the defaults. The settings'
// dconf keys (under /apps/parrotfish-keyboard/) are listed here once.
class KeyboardInfo : public QObject
{
    Q_OBJECT
    // The package version ("0.1.0~alpha109"; "dev" in a build of its own).
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit KeyboardInfo(QObject *parent = nullptr);

    QString version() const;

    // The diagnostics, as text to copy or send: the version, the system
    // (/etc/os-release), whether the debug file is on, Jolla's enabled and
    // active layouts, every setting ("-" if unset: the default) and what was learned, as counts per language
    // (LearningStore::summary). Nothing the user typed.
    Q_INVOKABLE QString diagnostics() const;
    // Unsets every key under /apps/parrotfish-keyboard/, as on a new
    // installation: the settings (the typing languages too), the hidden
    // ones (the sticky key's sign, the last language) and keys of older
    // versions. The keyboard and the settings pages show the defaults at
    // once. Nothing learned is touched, nor the debug file.
    Q_INVOKABLE void resetSettings() const;

    // The settings' dconf keys, without the path.
    static QStringList settingKeys();
};
