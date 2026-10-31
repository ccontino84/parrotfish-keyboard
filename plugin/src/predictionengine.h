// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/engine.h"

#include <QAbstractListModel>
#include <QJSValue>
#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>

// The engine as Jolla's PreeditHandler expects it: a list model with a
// `text` role and a predictionsChanged() signal. The handler sets `preedit`
// (the word being typed) and calls setContext() with the text before the
// cursor; with no word being typed, the model holds next-word suggestions.
class PredictionEngine : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString preedit READ preedit WRITE setPreedit NOTIFY preeditChanged)
    // The typing languages: dictionary codes separated by spaces ("en it
    // pt_BR"); empty: the system language (English if there is no
    // dictionary for it). Codes without a dictionary are skipped.
    Q_PROPERTY(QString languages READ languages WRITE setLanguages NOTIFY languagesChanged)
    // The layout's language code (EN, IT, DE): a weak hint for the start.
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // The language the last text was in (en, it, de; empty: none), used as the
    // starting lean for an empty field (plan §5, starting point 2).
    Q_PROPERTY(QString rememberedLanguage READ rememberedLanguage WRITE setRememberedLanguage
               NOTIFY rememberedLanguageChanged)
    // The language the text seems to be in (en, it, de), and how sure.
    Q_PROPERTY(QString leadingLanguage READ leadingLanguage NOTIFY leadingLanguageChanged)
    Q_PROPERTY(double confidence READ confidence NOTIFY leadingLanguageChanged)
    // Autocorrect strength: "off", "gentle", "normal" or "strong".
    Q_PROPERTY(QString strength READ strength WRITE setStrength NOTIFY strengthChanged)
    // Hidden tuning (plan §5): "name=value name=value", any EngineParams
    // field (see the engine headers); unknown names are ignored.
    Q_PROPERTY(QString tuning READ tuning WRITE setTuning NOTIFY tuningChanged)
    // For the slips.log header: the layout's settings, e.g. "QWERTY portrait
    // height=90" (no spaces inside a setting).
    Q_PROPERTY(QString slipsConfig READ slipsConfig WRITE setSlipsConfig NOTIFY slipsConfigChanged)
    // The word Space or punctuation would put in place of the word being
    // typed; empty if it stays as typed. Always the first suggestion.
    Q_PROPERTY(QString autocorrection READ autocorrection NOTIFY predictionsChanged)

public:
    enum Roles { TextRole = Qt::UserRole + 1 };

    explicit PredictionEngine(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString preedit() const { return m_preedit; }
    void setPreedit(const QString &preedit);
    QString languages() const { return m_languages; }
    void setLanguages(const QString &languages);
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString leadingLanguage() const;
    double confidence() const;
    QString rememberedLanguage() const { return m_remembered; }
    void setRememberedLanguage(const QString &language);
    QString strength() const { return m_strength; }
    void setStrength(const QString &strength);
    QString autocorrection() const { return m_autocorrection; }
    QString tuning() const { return m_tuning; }
    void setTuning(const QString &tuning);
    QString slipsConfig() const { return m_slipsConfig; }
    void setSlipsConfig(const QString &config);

    // The field's text before the cursor changed (or a new word begins):
    // updates the language weights and, while no word is being typed, the
    // next-word suggestions.
    Q_INVOKABLE void setContext(const QString &textBefore);
    // Letter keys of the current layout: [{ch, x, y, width, height}], centres
    // in any one coordinate system. Returns how many keys were taken. The
    // engine is rebuilt only when they differ from the keys it has.
    Q_INVOKABLE int setKeys(const QJSValue &keys);
    // An uncertain tap on `lifted` that started on `initial`: the letter to
    // type after `preedit`.
    Q_INVOKABLE QString resolveTap(const QString &preedit, const QString &lifted,
                                   const QString &initial) const;
    // The user undid the autocorrection of `typed`. After the second undo
    // (the first for a Strong fix of a valid word) it isn't corrected again
    // in this session.
    Q_INVOKABLE void rejectCorrection(const QString &typed);
    // The user kept an autocorrection of `typed` or picked a correction of it
    // from the bar: its undos are forgotten.
    Q_INVOKABLE void acceptCorrection(const QString &typed);
    // With the debug file only: the user kept an autocorrection ("kept"),
    // undid it ("undone") or picked a correction from the bar ("picked"); a
    // Strong fix of a valid word is "strong kept" or "strong undone", with
    // " margin=5.3" (nats);
    // appends the kind of slip (a letter pair for a wrong key, never the
    // words) to slips.log. A header line names the settings whenever they
    // change (and each day), with how many words were typed under the old.
    Q_INVOKABLE void logSlips(const QString &typed, const QString &meant, const QString &how);
    // With the debug file only: a word typed letter by letter was committed
    // (counted in slips.log in batches, and when the settings change).
    Q_INVOKABLE void countWord();
    // Whether `character` (its first code point) is a letter. Jolla's
    // KeyboardSupport.isLetter is missing before Sailfish OS 5.2.
    Q_INVOKABLE bool isLetter(const QString &character) const;
    // A diagnostic line for the journal, only while debugging is on. Never
    // pass typed text.
    Q_INVOKABLE void log(const QString &message) const;

signals:
    void preeditChanged();
    void languagesChanged();
    void languageChanged();
    void leadingLanguageChanged();
    void strengthChanged();
    void rememberedLanguageChanged();
    void tuningChanged();
    void slipsConfigChanged();
    void predictionsChanged();

private:
    void loadLanguages();
    void loadFallbackLayout();
    void applyParams();
    void update();
    QString slipsHeader() const;
    void appendSlips(QStringList lines);

    parrotfish::Engine m_engine;
    QString m_dataDir;
    QString m_preedit;
    QString m_languages;
    QString m_language;
    QString m_context;
    QString m_remembered;
    QStringList m_items;
    QString m_strength = QStringLiteral("normal");
    QString m_autocorrection;
    QString m_tuning;
    QHash<QString, int> m_undos;   // lowercase typed word → undos in a row
    // Strong's autocorrections of valid words: lowercase typed word →
    // (lowercase correction, margin in nats), for slips.log and undo.
    QHash<QString, QPair<QString, double>> m_strongFixes;
    bool m_layoutKeys = false;   // keys came from the real layout
    std::vector<parrotfish::Key> m_keys;   // the last keys from the layout
    bool m_debug = false;
    QString m_slipsConfig;
    QString m_slipsHeader;   // the last header written to slips.log
    int m_slipsWords = 0;    // words typed since the last count in slips.log
};
