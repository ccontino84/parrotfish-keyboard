// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// The typing languages Parrotfish has, and what each brings: its name,
// whether it is experimental, the accented letters its long-press menus
// show, its punctuation signs. From languages.json in the data folder (written by
// tools/data/language-list.py from data/rules/LANG.language), only the
// languages whose dictionary is installed. One copy for the plugin, the
// layout and the settings (a QML singleton, "Languages").
//
// The dconf key /apps/parrotfish-keyboard/languages holds the chosen codes
// separated by spaces; empty means the system language (or the one it
// stands for: no -> nb), else the fallback language (English).
class Languages : public QObject
{
    Q_OBJECT
    // [{code, name, experimental}], sorted by name.
    Q_PROPERTY(QVariantList all READ all CONSTANT)
    // Every enabled language is searched on each keystroke (about 6 ms each
    // at worst on a phone, estimated from 3 languages), and close languages
    // compete for the same words. Beyond this many, the settings say so.
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
    // The comma's and the period's long-press menus: {comma, period}.
    // Jolla's (: , ; and ! . ?) plus the languages' signs.
    Q_INVOKABLE QVariantMap punctuation(const QStringList &codes) const;

    // For tests: as if the system locale were `name` ("fr_CA").
    Q_INVOKABLE void setSystemLocale(const QString &name);

private:
    struct Language {
        QString code, name, menuLetters, commaExtra, periodBefore, periodAfter;
        bool experimental = false, fallback = false;
        QStringList systemLanguages;
    };
    QList<Language> m_languages;   // sorted by name
    QVariantList m_all;
    QString m_locale;

    const Language *find(const QString &code) const;
};
