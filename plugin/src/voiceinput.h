// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "voicetext.h"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// Voice input through the helper process (parrotfish-voice): QML singleton
// Voice. The helper quits by itself when idle; the text is in VoiceText.
class VoiceInput : public QObject
{
    Q_OBJECT
    // The helper is installed and the model downloaded (refresh() checks again).
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    // "off", "listening", "finishing" (the last phrase is being decoded) or
    // "paused" (the text is kept: start() continues, stop() finishes).
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    // The model is loaded.
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    // The text recognised so far: fixedText (words, joined), then openText,
    // which can still change at the next pause.
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QString fixedText READ fixedText NOTIFY textChanged)
    Q_PROPERTY(QStringList words READ words NOTIFY textChanged)
    Q_PROPERTY(QString openText READ openText NOTIFY textChanged)
    // The selected words (indexes in words, inclusive; -1 without one).
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY textChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY textChanged)
    // The next phrase replaces the selection.
    Q_PROPERTY(bool replacing READ replacing NOTIFY textChanged)
    // What undo() changes when openText is empty: it removes words undoFrom
    // to undoTo (exclusive) and puts undoRestores there.
    Q_PROPERTY(int undoFrom READ undoFrom NOTIFY textChanged)
    Q_PROPERTY(int undoTo READ undoTo NOTIFY textChanged)
    Q_PROPERTY(QStringList undoRestores READ undoRestores NOTIFY textChanged)
    // The microphone level, 0-1.
    Q_PROPERTY(qreal level READ level NOTIFY levelChanged)

public:
    explicit VoiceInput(QObject *parent = nullptr);
    ~VoiceInput() override;

    bool available() const { return m_available; }
    QString state() const { return m_state; }
    bool ready() const { return m_ready; }
    QString text() const { return m_text.text(); }
    QString fixedText() const { return m_text.fixedText(); }
    QStringList words() const { return m_text.words(); }
    QString openText() const { return m_text.openText(); }
    int selectionStart() const { return m_text.selectionStart(); }
    int selectionEnd() const { return m_text.selectionEnd(); }
    bool replacing() const { return m_text.replacing(); }
    int undoFrom() const { return m_text.undoFrom(); }
    int undoTo() const { return m_text.undoTo(); }
    QStringList undoRestores() const { return m_text.undoRestores(); }
    qreal level() const { return m_level; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void prepare();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void cancel();
    // Removes openText if any, else takes the last change back: a phrase,
    // a replacement or a deletion.
    Q_INVOKABLE void undo();
    // Selects words from-to (inclusive, either order); the phrase after the
    // next start() replaces them.
    Q_INVOKABLE void select(int from, int to);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void deleteSelection();

signals:
    void availableChanged();
    void stateChanged();
    void readyChanged();
    void textChanged();
    void levelChanged();
    void finished(const QString &text);
    void failed(const QString &message);

private:
    bool ensureRunning();
    void send(const char *command);
    void readLines();
    void handle(const QString &line);
    void setState(const QString &state);
    void setReady(bool ready);
    void endFinishing();

    QProcess *m_process = nullptr;
    QByteArray m_buffer;
    VoiceText m_text;
    bool m_available = false;
    QString m_state = QStringLiteral("off");
    bool m_ready = false;
    bool m_modelFailed = false;
    bool m_pausing = false;
    bool m_resumeWanted = false;
    bool m_undoWanted = false;   // undo() while pausing: once paused
    qreal m_level = 0;
};
