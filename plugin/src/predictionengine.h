// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "learningstore.h"
#include "storewriter.h"
#include "parrotfish/engine.h"

#include <QAbstractListModel>
#include <QJSValue>
#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>

#include <memory>

// The engine as Jolla's PreeditHandler expects it: a list model with a
// `text` role and a predictionsChanged() signal. The handler sets `preedit`
// (the word being typed) and calls setContext() with the text before the
// cursor; with no word being typed, the model holds next-word suggestions.
class PredictionEngine : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString preedit READ preedit WRITE setPreedit NOTIFY preeditChanged)
    // Set before `preedit`: per letter, "" or the tap's other key and "1" if
    // the letter shown is where the finger lifted ("e1"), "0" if where it
    // touched down. Ignored if its length differs.
    Q_PROPERTY(QStringList preeditKeys READ preeditKeys WRITE setPreeditKeys NOTIFY preeditKeysChanged)
    // The typing languages ("en it pt_BR"); empty: the system language, else
    // English. Codes without a dictionary are skipped.
    Q_PROPERTY(QString languages READ languages WRITE setLanguages NOTIFY languagesChanged)
    // The layout's script ("Latn", "Cyrl", "Grek"): only its languages are loaded.
    Q_PROPERTY(QString script READ script WRITE setScript NOTIFY scriptChanged)
    // The layout's language code (EN, IT, DE): a weak hint for the start.
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // The language the last text was in: the starting lean for an empty field.
    Q_PROPERTY(QString rememberedLanguage READ rememberedLanguage WRITE setRememberedLanguage
               NOTIFY rememberedLanguageChanged)
    // The language the text seems to be in (en, it, de), and how sure.
    Q_PROPERTY(QString leadingLanguage READ leadingLanguage NOTIFY leadingLanguageChanged)
    Q_PROPERTY(double confidence READ confidence NOTIFY leadingLanguageChanged)
    // Autocorrect strength: "off", "gentle", "normal" or "strong".
    Q_PROPERTY(QString strength READ strength WRITE setStrength NOTIFY strengthChanged)
    // For the slips.log header: the layout's settings, e.g. "QWERTY portrait
    // height=90" (no spaces inside a setting).
    Q_PROPERTY(QString slipsConfig READ slipsConfig WRITE setSlipsConfig NOTIFY slipsConfigChanged)
    // Whether next words at a sentence start are capitalised (the keyboard's
    // automatic capitals); otherwise they keep the dictionary's case.
    Q_PROPERTY(bool capitals READ capitals WRITE setCapitals NOTIFY capitalsChanged)
    // The word Space or punctuation would put in place of the word being
    // typed; empty if it stays as typed. Always the first suggestion.
    Q_PROPERTY(QString autocorrection READ autocorrection NOTIFY predictionsChanged)
    // Whether the debug file exists (read once at start): log details.
    Q_PROPERTY(bool debug READ debug CONSTANT)
    // Learning: "on", "privacy" (only the undo memory) or "off" (nothing
    // learned or used; what was learned is kept).
    Q_PROPERTY(QString learning READ learning WRITE setLearning NOTIFY learningChanged)
    // While true, nothing is learned or stored; what was learned is still used.
    Q_PROPERTY(bool learnPaused READ learnPaused WRITE setLearnPaused NOTIFY learnPausedChanged)

public:
    enum Roles { TextRole = Qt::UserRole + 1 };

    explicit PredictionEngine(QObject *parent = nullptr);
    ~PredictionEngine() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString preedit() const { return m_preedit; }
    void setPreedit(const QString &preedit);
    QStringList preeditKeys() const { return m_preeditKeys; }
    void setPreeditKeys(const QStringList &keys);
    QString languages() const { return m_languages; }
    void setLanguages(const QString &languages);
    QString script() const { return m_script; }
    void setScript(const QString &script);
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString leadingLanguage() const;
    double confidence() const;
    QString rememberedLanguage() const { return m_remembered; }
    void setRememberedLanguage(const QString &language);
    QString strength() const { return m_strength; }
    void setStrength(const QString &strength);
    bool capitals() const { return m_capitals; }
    void setCapitals(bool on);
    QString autocorrection() const { return m_autocorrection; }
    bool debug() const { return m_debug; }
    QString slipsConfig() const { return m_slipsConfig; }
    void setSlipsConfig(const QString &config);
    QString learning() const { return m_learning; }
    void setLearning(const QString &mode);
    bool learnPaused() const { return m_learnPaused; }
    void setLearnPaused(bool paused);

    // The text before the cursor changed or a new word begins: updates the
    // language weights and the next words.
    Q_INVOKABLE void setContext(const QString &textBefore);
    // The layout's letter keys, [{ch, x, y, width, height}] (centres, any one
    // coordinate system). Returns how many were taken.
    Q_INVOKABLE int setKeys(const QJSValue &keys);
    // An uncertain tap on `lifted` that started on `initial`: the letter to
    // type after `preedit`.
    Q_INVOKABLE QString resolveTap(const QString &preedit, const QString &lifted,
                                   const QString &initial) const;
    // The autocorrection of `typed` was undone: after two (a Strong fix of a
    // valid word: one), not corrected again in that language.
    Q_INVOKABLE void rejectCorrection(const QString &typed);
    // An autocorrection of `typed` was kept or picked: its undos are forgotten.
    Q_INVOKABLE void acceptCorrection(const QString &typed);
    // With the debug file only: appends the kind of slip (never the words)
    // to slips.log; `how`: kept, undone, undone late, redone, picked, manual,
    // strong kept, strong undone.
    Q_INVOKABLE void logSlips(const QString &typed, const QString &meant, const QString &how);
    // With the debug file only: a word typed letter by letter was committed.
    Q_INVOKABLE void countWord();
    // With the debug file only: a suggestion was used, `how` "picked",
    // "next" or "fixed"; counted in slips.log (numbers only).
    Q_INVOKABLE void countUse(const QString &how, const QString &word);
    // Whether `character` (its first code point) is a letter. Jolla's
    // KeyboardSupport.isLetter is missing before Sailfish OS 5.2.
    Q_INVOKABLE bool isLetter(const QString &character) const;
    // A journal line while debugging is on. Never pass typed text.
    Q_INVOKABLE void log(const QString &message) const;
    // Updates the suggestions now instead of on the next event loop pass.
    Q_INVOKABLE void flush();

    // `text` was committed after `textBefore`: its words are learned once
    // kept (the next commit or the end of the text).
    Q_INVOKABLE void committed(const QString &textBefore, const QString &text);
    // The autocorrection to `corrected` was undone: the word kept is `typed`.
    Q_INVOKABLE void replaceKept(const QString &corrected, const QString &typed);
    // The last word committed is being typed again: not learned as it was.
    // Only if the kept word is `word`, when given.
    Q_INVOKABLE void dropKept(const QString &word = QString());
    // The text ended: saves what was learned. `closed`: the keyboard closed,
    // the time for Learned::maintain.
    Q_INVOKABLE void endText(bool closed = false);
    // The keyboard opened: maintenance still due from an unclean close.
    Q_INVOKABLE void opened();
    // The settings page changed learning.db: read it again.
    Q_INVOKABLE void reloadLearned();

signals:
    void preeditChanged();
    void preeditKeysChanged();
    void languagesChanged();
    void scriptChanged();
    void languageChanged();
    void leadingLanguageChanged();
    void strengthChanged();
    void capitalsChanged();
    void rememberedLanguageChanged();
    void slipsConfigChanged();
    void learningChanged();
    void learnPausedChanged();
    void predictionsChanged();

private:
    void loadLanguages();
    void loadFallbackLayout();
    void applyParams();
    void update();
    QString slipsHeader() const;
    void appendSlips(QStringList lines);
    QString fixLanguage(const QString &typed) const;
    void learnKept();
    void saveLearned();
    void maintainLearned();

    parrotfish::Engine m_engine;
    QString m_dataDir;
    QString m_preedit;
    QStringList m_preeditKeys;
    QString m_languages;
    QString m_script = QStringLiteral("Latn");
    QString m_language;
    QString m_context;
    std::vector<double> m_contextPrior;   // the starting leans m_context was read with
    bool m_contextValid = false;   // the engine has read m_context (setContext)
    bool m_nextValid = false;      // m_items are the next words for m_context
    QString m_remembered;
    QStringList m_items;
    QString m_strength = QStringLiteral("normal");
    bool m_capitals = true;
    QString m_autocorrection;
    // (language, lowercase typed word) → undos in a row
    QHash<LearningStore::UndoKey, int> m_undos;
    std::unique_ptr<LearningStore> m_store;   // keeps m_undos between sessions (reads)
    std::unique_ptr<StoreWriter> m_writer;    // all writes, on a thread of their own
    // Recent autocorrections: lowercase typed word → the leading language then.
    QHash<QString, QString> m_fixLanguages;
    // Strong's autocorrections of valid words: lowercase typed word →
    // (lowercase correction, margin in nats), for slips.log and undo.
    QHash<QString, QPair<QString, double>> m_strongFixes;
    bool m_layoutKeys = false;   // keys came from the real layout
    std::vector<parrotfish::Key> m_keys;   // the last keys from the layout
    bool m_debug = false;
    bool m_updatePending = false;   // preedit changed, update() not run yet
    QString m_slipsConfig;
    QString m_slipsHeader;   // the last header written to slips.log
    int m_slipsWords = 0;    // words typed since the last count in slips.log
    // Since the last words line: suggestions used, and of them from learning.
    struct Uses { int picks = 0, picksLearned = 0, next = 0, nextLearned = 0, fixes = 0, fixesLearned = 0; } m_uses;
    QString wordsLine(int words);

    QString m_learning = QStringLiteral("on");
    bool m_learnPaused = false;
    parrotfish::Learned m_learned;
    // The last word committed, learned once kept (its language: the one when
    // learned, if unsure when typed).
    struct Kept {
        bool valid = false;
        std::u32string before, word;
        std::string language;
        bool sure = true;
    } m_kept;
    int m_keptSinceSave = 0;
    // The last searches' results, for Backspace back to a prefix. Cleared
    // when the engine changes.
    QHash<QString, parrotfish::Result> m_results;
    QString resultKey() const;
};
