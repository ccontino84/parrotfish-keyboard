// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// The typing languages and what each brings, from languages.json
// (tools/data/language-list.py), and where their dictionaries are: from a
// language package (the data directory) or downloaded (downloadDir()). A
// QML singleton, "Languages", the same object as instance().
//
// /apps/parrotfish-keyboard/languages: codes separated by spaces; empty:
// the system language (no → nb), else English. A layout types the
// languages of its script (Latn, Cyrl, Grek) only.
//
// A dictionary counts with its record beside it, CODE.pfd.sha256 (the
// file's SHA-256 in hex): a download writes it last.
class Languages : public QObject
{
    Q_OBJECT
    // [{code, name, script, experimental, packaged, downloaded, installed,
    // source, update, size}], sorted by name. source: "download" or "package", the
    // file the keyboard tries first, or "". update: a download (size
    // bytes) would bring the dictionary of this release.
    Q_PROPERTY(QVariantList all READ all NOTIFY changed)
    // Counts the rescans that changed something, for bindings on functions.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    // The language of the system locale, installed or not, or "".
    Q_PROPERTY(QString systemLanguage READ systemLanguage NOTIFY changed)
    // An installed language has an update (all's update).
    Q_PROPERTY(bool updatesAvailable READ updatesAvailable NOTIFY changed)
    // Beyond this many languages, the settings warn (each costs up to ~6 ms
    // a keystroke on a phone, and close languages compete).
    Q_PROPERTY(int comfortable READ comfortable CONSTANT)

public:
    explicit Languages(QObject *parent = nullptr);
    static Languages &instance();

    // Where downloaded dictionaries go.
    static QString downloadDir();

    QVariantList all() const { return m_all; }
    int revision() const { return m_revision; }
    QString systemLanguage() const;
    bool updatesAvailable() const;
    int comfortable() const { return 4; }

    // Reads the dictionaries' files again. A package's dictionary without a
    // record (packages before 1.4) is never reported as old.
    Q_INVOKABLE void refresh();
    // The dictionary files of `code` to try in turn: first the ones that
    // are this release's, a download before a package's.
    QStringList dictionaryFiles(const QString &code) const;
    Q_INVOKABLE bool isInstalled(const QString &code) const;
    // The installed languages a layout of `script` types with for the dconf
    // value: as PredictionEngine, the default ones when none is installed.
    Q_INVOKABLE QStringList typing(const QString &value, const QString &script = QStringLiteral("Latn")) const;
    // The installed languages of every script's layouts (typing() of each),
    // only the scripts of `layouts` (Jolla's enabled_layouts) if given.
    Q_INVOKABLE QStringList typingAll(const QString &value, const QVariant &layouts = QVariant()) const;
    // The scripts of the Parrotfish layouts among `layouts`; none: all.
    Q_INVOKABLE QStringList scriptsOn(const QVariant &layouts) const;
    // The installed ones of `codes`.
    Q_INVOKABLE QStringList installedOf(const QStringList &codes) const;

    // The typing languages of the dconf value in `script` (known codes, in
    // the order of `all`), or the script's default.
    Q_INVOKABLE QStringList selected(const QString &value, const QString &script = QStringLiteral("Latn")) const;
    // The typing languages of every script, as the settings show them.
    Q_INVOKABLE QStringList selectedAll(const QString &value) const;
    // The default in `script`. Latin: the system language's
    // (QLocale::system()) if it is written in it and installed, else the
    // fallback's, else the first installed; with none installed, the
    // system language's, else the fallback. Other scripts: every installed
    // language of the script, the system language's first (the layout's
    // switch key picks among them).
    QStringList defaultLanguages(const QString &script = QStringLiteral("Latn")) const;
    // The script of a language ("Latn" if unknown).
    Q_INVOKABLE QString script(const QString &code) const;
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

signals:
    void changed();

private:
    struct Language {
        QString code, name, script, menuLetters, extraKeys, commaExtra, periodBefore, periodAfter;
        bool experimental = false, fallback = false;
        QStringList systemLanguages;
        // languages.json's "download": the compressed file's size, and the
        // dictionary's SHA-256 (this release's).
        qint64 downloadSize = 0;
        QString releaseHash;
        // Files (refresh()): their recorded SHA-256, "" for none.
        bool packaged = false, downloaded = false;
        QString packageHash, downloadHash;
    };
    QList<Language> m_languages;   // sorted by name
    QVariantList m_all;
    QString m_dataDir;
    QString m_locale;
    int m_revision = 0;

    const Language *find(const QString &code) const;
    QString packagePath(const QString &code) const;
    void updateAll();
};
