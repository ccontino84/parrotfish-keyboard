// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QObject>
#include <QStringList>

// For the settings pages: the version, the diagnostics, the reset.
class KeyboardInfo : public QObject
{
    Q_OBJECT
    // The package version ("1.1.0", "1.1.0~alpha1"; "dev" in a build of its own).
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit KeyboardInfo(QObject *parent = nullptr);

    QString version() const;

    // The diagnostics as text: versions, layouts, settings ("-": unset) and
    // learned counts. Nothing the user typed.
    Q_INVOKABLE QString diagnostics() const;
    // Unsets every key under /apps/parrotfish-keyboard/; nothing learned is touched.
    Q_INVOKABLE void resetSettings() const;

    // The settings' dconf keys, without the path.
    static QStringList settingKeys();
};
