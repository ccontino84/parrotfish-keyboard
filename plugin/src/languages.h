// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// The installed typing languages and what each brings, from languages.json
// (tools/data/language-list.py). A QML singleton, "Languages".
//
// /apps/parrotfish-keyboard/languages: codes separated by spaces; empty:
// the system language (no → nb), else English.
class Languages : public QObject
{
    Q_OBJECT
    // [{code, name, experimental}], sorted by name.
    Q_PROPERTY(QVariantList all READ all CONSTANT)
    // Beyond this many languages, the settings warn (each costs up to ~6 ms
    // a keystroke on a phone, and close languages compete).
    Q_PROPERTY(int comfortable READ comfortable CONSTANT)

public:
    explicit Languages(QObject *parent = nullptr);
    // The one the plugin uses (loaded once).
    static const Languages &instance();

    QVariantList all() const { return m_all; }
    int comfortable() const { return 4; }

    // The typing languages of the dconf value (known codes, in the order
    // of `all`), or the default.
    Q_INVOKABLE QStringList selected(const QString &value) const;
    // The default: the system language's (QLocale::system()), else the
    // fallback.
    QStringList defaultLanguages() const;
    // Their names, separated by commas.
    Q_INVOKABLE QString names(const QStringList &codes) const;
    // `codes` with `code` switched on or off, as the dconf value.
    Q_INVOKABLE QString toggled(const QStringList &codes, const QString &code) const;
    // The accented letters of the languages' long-press menus, each once.
    Q_INVOKABLE QString menuLetters(const QStringList &codes) const;
    // The letters of the language's language keys (extra-keys), or "".
    Q_INVOKABLE QString extraKeys(const QString &code) const;
    // The comma's and the period's long-press menus: {comma, period}.
    // Jolla's (: , ; and ! . ?) plus the languages' signs.
    Q_INVOKABLE QVariantMap punctuation(const QStringList &codes) const;

    // For tests: as if the system locale were `name` ("fr_CA").
    Q_INVOKABLE void setSystemLocale(const QString &name);

private:
    struct Language {
        QString code, name, menuLetters, extraKeys, commaExtra, periodBefore, periodAfter;
        bool experimental = false, fallback = false;
        QStringList systemLanguages;
    };
    QList<Language> m_languages;   // sorted by name
    QVariantList m_all;
    QString m_locale;

    const Language *find(const QString &code) const;
};
