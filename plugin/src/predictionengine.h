// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#pragma once

#include "learningstore.h"
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
    // Set before `preedit`: for each of its letters, "" or the tap's other
    // key and whether the letter shown is where the finger lifted ("e1": the
    // tap also touched e, and lifted on the letter shown; "e0": it lifted on
    // e, and the letter shown is where it touched down). The search weighs
    // both keys with the whole word. Ignored if its length differs.
    Q_PROPERTY(QStringList preeditKeys READ preeditKeys WRITE setPreeditKeys NOTIFY preeditKeysChanged)
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
    // Whether the debug file exists (read once at start): log details.
    Q_PROPERTY(bool debug READ debug CONSTANT)
    // Learning: "on" (word counts, new words, word
    // pairs learned and used), "privacy" (only the undo memory: nothing
    // typed shows up in the bar elsewhere) or "off" (nothing learned or
    // used, the undo memory included). What was learned is kept until
    // cleared.
    Q_PROPERTY(QString learning READ learning WRITE setLearning NOTIFY learningChanged)

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
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString leadingLanguage() const;
    double confidence() const;
    QString rememberedLanguage() const { return m_remembered; }
    void setRememberedLanguage(const QString &language);
    QString strength() const { return m_strength; }
    void setStrength(const QString &strength);
    QString autocorrection() const { return m_autocorrection; }
    bool debug() const { return m_debug; }
    QString tuning() const { return m_tuning; }
    void setTuning(const QString &tuning);
    QString slipsConfig() const { return m_slipsConfig; }
    void setSlipsConfig(const QString &config);
    QString learning() const { return m_learning; }
    void setLearning(const QString &mode);

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
    // in text of the same language: the language the text was in when it
    // was corrected (kept between sessions in learning.db, up to
    // LearningStore::MaxUndos words).
    Q_INVOKABLE void rejectCorrection(const QString &typed);
    // The user kept an autocorrection of `typed` or picked a correction of it
    // from the bar: its undos in that language are forgotten.
    Q_INVOKABLE void acceptCorrection(const QString &typed);
    // With the debug file only: the user kept an autocorrection ("kept"),
    // undid it ("undone"; "undone late" when the corrected word was changed
    // after more typing), took the undo back ("redone"), picked a
    // correction from the bar ("picked") or
    // changed a word by hand ("manual": slips only, no "other"); a
    // Strong fix of a valid word is "strong kept" or "strong undone", with
    // " margin=5.3" (nats);
    // appends the kind of slip (a letter pair for a wrong key, never the
    // words) to slips.log. A header line names the settings whenever they
    // change (and each day), with how many words were typed under the old.
    Q_INVOKABLE void logSlips(const QString &typed, const QString &meant, const QString &how);
    // With the debug file only: a word typed letter by letter was committed
    // (counted in slips.log in batches, and when the settings change).
    Q_INVOKABLE void countWord();
    // With the debug file only: a suggestion was used, `how` "picked" (from
    // the bar while typing), "next" (from the bar before a letter) or
    // "fixed" (autocorrected at Space or a sign). Counted on the words line
    // of slips.log with how many came from learning: a learned new word
    // picked or fixed to, a next word the user typed after the same word
    // before (numbers only).
    Q_INVOKABLE void countUse(const QString &how, const QString &word);
    // Whether `character` (its first code point) is a letter. Jolla's
    // KeyboardSupport.isLetter is missing before Sailfish OS 5.2.
    Q_INVOKABLE bool isLetter(const QString &character) const;
    // A diagnostic line for the journal, only while debugging is on. Never
    // pass typed text.
    Q_INVOKABLE void log(const QString &message) const;
    // Brings the suggestions and `autocorrection` up to date with preedit
    // now (they follow it on the next pass of the event loop otherwise).
    Q_INVOKABLE void flush();

    // Learning. `text` was committed after `textBefore` (a word with its
    // space or sign, a sign, a picked word): its words are learned once
    // kept, i.e. at the next commit or the end of the text, unless undone
    // first.
    Q_INVOKABLE void committed(const QString &textBefore, const QString &text);
    // The autocorrection to `corrected` was undone: the word kept is `typed`.
    Q_INVOKABLE void replaceKept(const QString &corrected, const QString &typed);
    // The last word committed is being typed again (Backspace into it, or
    // typing on at its end): not learned as it was.
    Q_INVOKABLE void dropKept();
    // The text ended (the keyboard closed, the field was emptied): a new
    // word counts once per text. Saves what was learned.
    Q_INVOKABLE void endText();
    // The settings page changed learning.db (a word forgotten, all
    // cleared): read it again, dropping what wasn't saved.
    Q_INVOKABLE void reloadLearned();

signals:
    void preeditChanged();
    void preeditKeysChanged();
    void languagesChanged();
    void languageChanged();
    void leadingLanguageChanged();
    void strengthChanged();
    void rememberedLanguageChanged();
    void tuningChanged();
    void slipsConfigChanged();
    void learningChanged();
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

    parrotfish::Engine m_engine;
    QString m_dataDir;
    QString m_preedit;
    QStringList m_preeditKeys;
    QString m_languages;
    QString m_language;
    QString m_context;
    std::vector<double> m_contextPrior;   // the starting leans m_context was read with
    bool m_contextValid = false;   // the engine has read m_context (setContext)
    bool m_nextValid = false;      // m_items are the next words for m_context
    QString m_remembered;
    QStringList m_items;
    QString m_strength = QStringLiteral("normal");
    QString m_autocorrection;
    QString m_tuning;
    // (language, lowercase typed word) → undos in a row
    QHash<LearningStore::UndoKey, int> m_undos;
    std::unique_ptr<LearningStore> m_store;   // keeps m_undos between sessions
    // Recent autocorrections: lowercase typed word → the text's leading
    // language when it was corrected (undos are kept per language).
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
    parrotfish::Learned m_learned;
    // The last word committed, learned once kept: the text before it, the
    // word, the text's language then.
    struct Kept {
        bool valid = false;
        std::u32string before, word;
        std::string language;
    } m_kept;
    int m_keptSinceSave = 0;
    // The last searches' results (Backspace back to a prefix shows its
    // suggestions without searching again): by preedit, keys, context and
    // learned data. Cleared when the engine changes.
    QHash<QString, parrotfish::Result> m_results;
    QString resultKey() const;
};
