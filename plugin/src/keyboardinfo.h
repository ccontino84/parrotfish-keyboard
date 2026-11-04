// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <QObject>
#include <QStringList>

// For the settings page: the installed version and what a bug report
// needs (the diagnostics page). Nothing the user typed: counts only.
class KeyboardInfo : public QObject
{
    Q_OBJECT
    // The package version ("0.1.0~alpha109"; "dev" in a build of its own).
    Q_PROPERTY(QString version READ version CONSTANT)
    // Whether the debug file exists: the keyboard logs details and writes
    // slips.log.
    Q_PROPERTY(bool debug READ debug CONSTANT)

public:
    explicit KeyboardInfo(QObject *parent = nullptr);

    QString version() const;
    bool debug() const;
    // learning.db per language (LearningStore::summary).
    Q_INVOKABLE QStringList learningSummary() const;
};
