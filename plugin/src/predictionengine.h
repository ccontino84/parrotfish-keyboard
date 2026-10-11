// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "parrotfish/engine.h"

#include <QAbstractListModel>
#include <QJSValue>
#include <QHash>
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
    // The layout's language code (EN, IT, DE): a weak hint for the start.
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // The language the last text was in (en, it, de; empty: none), used as the
    // starting lean for an empty field (plan §5, starting point 2).
    Q_PROPERTY(QString rememberedLanguage READ rememberedLanguage WRITE setRememberedLanguage
               NOTIFY rememberedLanguageChanged)
    // The language the text seems to be in (en, it, de), and how sure.
    Q_PROPERTY(QString leadingLanguage READ leadingLanguage NOTIFY leadingLanguageChanged)
    Q_PROPERTY(double confidence READ confidence NOTIFY leadingLanguageChanged)
    // Autocorrect strength: "off", "gentle" or "normal".
    Q_PROPERTY(QString strength READ strength WRITE setStrength NOTIFY strengthChanged)
    // Hidden tuning (plan §5): "name=value name=value", any EngineParams
    // field (see the engine headers); unknown names are ignored.
    Q_PROPERTY(QString tuning READ tuning WRITE setTuning NOTIFY tuningChanged)
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
    // it isn't corrected again in this session.
    Q_INVOKABLE void rejectCorrection(const QString &typed);
    // The user kept an autocorrection of `typed` or picked a correction of it
    // from the bar: its undos are forgotten.
    Q_INVOKABLE void acceptCorrection(const QString &typed);
    // A diagnostic line for the journal, only while debugging is on. Never
    // pass typed text.
    Q_INVOKABLE void log(const QString &message) const;

signals:
    void preeditChanged();
    void languageChanged();
    void leadingLanguageChanged();
    void strengthChanged();
    void rememberedLanguageChanged();
    void tuningChanged();
    void predictionsChanged();

private:
    void loadFallbackLayout();
    void applyParams();
    void update();

    parrotfish::Engine m_engine;
    QString m_dataDir;
    QString m_preedit;
    QString m_language;
    QString m_context;
    QString m_remembered;
    QStringList m_items;
    QString m_strength = QStringLiteral("gentle");
    QString m_autocorrection;
    QString m_tuning;
    QHash<QString, int> m_undos;   // lowercase typed word → undos in a row
    bool m_layoutKeys = false;   // keys came from the real layout
    std::vector<parrotfish::Key> m_keys;   // the last keys from the layout
    bool m_debug = false;
};
