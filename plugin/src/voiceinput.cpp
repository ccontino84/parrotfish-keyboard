// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "voiceinput.h"

#include "voicepaths.h"

#include <QDir>
#include <QStandardPaths>

VoiceInput::VoiceInput(QObject *parent)
    : QObject(parent)
{
    refresh();
}

VoiceInput::~VoiceInput()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->closeWriteChannel();   // the helper quits at the end of its input
        m_process->waitForFinished(500);
    }
}

void VoiceInput::refresh()
{
    const bool available = QFileInfo(VoicePaths::helper()).isExecutable() && VoicePaths::modelReady();
    if (available == m_available)
        return;
    m_available = available;
    m_modelFailed = false;
    emit availableChanged();
}

bool VoiceInput::ensureRunning()
{
    if (!m_available)
        return false;
    if (m_process && m_process->state() != QProcess::NotRunning)
        return true;
    if (!m_process) {
        m_process = new QProcess(this);
        const QString cache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                              + QStringLiteral("/parrotfish-keyboard");
        QDir().mkpath(cache);
        m_process->setStandardErrorFile(cache + QStringLiteral("/voice.log"), QIODevice::Truncate);
        connect(m_process, &QProcess::readyReadStandardOutput, this, &VoiceInput::readLines);
        connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                this, [this](int, QProcess::ExitStatus) {
                    setReady(false);
                    if (m_state == QStringLiteral("paused"))
                        return;
                    if (m_pausing) {
                        endFinishing();
                        return;
                    }
                    const bool busy = m_state != QStringLiteral("off");
                    setState(QStringLiteral("off"));
                    if (busy)
                        emit failed(QStringLiteral("stopped"));
                });
    }
    m_buffer.clear();
    m_process->start(VoicePaths::helper(), QStringList() << VoicePaths::modelDir());
    return m_process->waitForStarted(2000);
}

void VoiceInput::send(const char *command)
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write(command);
        m_process->write("\n");
    }
}

void VoiceInput::prepare()
{
    ensureRunning();
}

void VoiceInput::start()
{
    if (m_state == QStringLiteral("finishing") && m_pausing) {
        m_resumeWanted = true;
        return;
    }
    const bool resume = m_state == QStringLiteral("paused");
    if ((m_state != QStringLiteral("off") && !resume) || !ensureRunning())
        return;
    if (m_modelFailed) {
        emit failed(QStringLiteral("model"));
        return;
    }
    if (!resume)
        m_text.clear();
    m_text.startRecording();
    emit textChanged();
    send(resume ? "continue" : "start");
    setState(QStringLiteral("listening"));
}

void VoiceInput::stop()
{
    m_resumeWanted = false;
    m_undoWanted = false;
    if (m_state == QStringLiteral("paused")) {
        setState(QStringLiteral("off"));
        emit finished(text());
        return;
    }
    if (m_state == QStringLiteral("finishing") && m_pausing) {
        m_pausing = false;   // finishes instead
        return;
    }
    if (m_state != QStringLiteral("listening"))
        return;
    send("stop");
    setState(QStringLiteral("finishing"));
}

void VoiceInput::pause()
{
    m_resumeWanted = false;
    if (m_state == QStringLiteral("listening")) {
        send("stop");
        setState(QStringLiteral("finishing"));
    }
    if (m_state == QStringLiteral("finishing"))
        m_pausing = true;
}

// The helper has sent the last phrase.
void VoiceInput::endFinishing()
{
    m_text.commitOpen();
    emit textChanged();
    if (m_pausing) {
        m_pausing = false;
        setState(QStringLiteral("paused"));
        if (m_undoWanted) {
            m_undoWanted = false;
            undo();
        }
        if (m_resumeWanted) {
            m_resumeWanted = false;
            start();
        }
        return;
    }
    m_text.clearSelection();
    setState(QStringLiteral("off"));
    emit finished(text());
}

void VoiceInput::cancel()
{
    m_pausing = false;
    m_resumeWanted = false;
    m_undoWanted = false;
    if (m_state == QStringLiteral("off"))
        return;
    if (m_state != QStringLiteral("paused"))
        send("cancel");
    setState(QStringLiteral("off"));
    m_text.clear();
    emit textChanged();
}

void VoiceInput::undo()
{
    if (m_state == QStringLiteral("finishing") && m_pausing) {
        m_undoWanted = true;   // the open phrase becomes fixed first
        return;
    }
    if (m_state != QStringLiteral("listening") && m_state != QStringLiteral("paused"))
        return;
    if (m_text.undo())
        emit textChanged();
}

void VoiceInput::select(int from, int to)
{
    m_text.select(from, to);
    emit textChanged();
}

void VoiceInput::clearSelection()
{
    if (m_text.clearSelection())
        emit textChanged();
}

void VoiceInput::deleteSelection()
{
    if (m_text.deleteSelection())
        emit textChanged();
}

void VoiceInput::readLines()
{
    m_buffer += m_process->readAllStandardOutput();
    int end;
    while ((end = m_buffer.indexOf('\n')) >= 0) {
        handle(QString::fromUtf8(m_buffer.left(end)));
        m_buffer.remove(0, end + 1);
    }
}

void VoiceInput::handle(const QString &line)
{
    const int space = line.indexOf(QLatin1Char(' '));
    const QString word = space < 0 ? line : line.left(space);
    const QString rest = space < 0 ? QString() : line.mid(space + 1);
    if (word == QStringLiteral("ready")) {
        setReady(true);
    } else if (word == QStringLiteral("level")) {
        m_level = rest.toDouble();
        emit levelChanged();
    } else if (word == QStringLiteral("fixed") || word == QStringLiteral("open")) {
        // "fixed|open <phrase> <text>"
        if (m_state == QStringLiteral("off") || m_state == QStringLiteral("paused"))
            return;
        const int space2 = rest.indexOf(QLatin1Char(' '));
        const int phrase = rest.left(space2 < 0 ? rest.size() : space2).toInt();
        const QString text = space2 < 0 ? QString() : rest.mid(space2 + 1);
        if (word == QStringLiteral("fixed"))
            m_text.fixed(phrase, text);
        else
            m_text.open(phrase, text);
        emit textChanged();
    } else if (word == QStringLiteral("final") || word == QStringLiteral("idle")) {
        // final repeats this recording's text; the edits are only here.
        if (m_state == QStringLiteral("finishing"))
            endFinishing();
    } else if (word == QStringLiteral("error")) {
        if (rest == QStringLiteral("model"))
            m_modelFailed = true;
        if (m_pausing) {
            endFinishing();
        } else if (m_state != QStringLiteral("off") && m_state != QStringLiteral("paused")) {
            setState(QStringLiteral("off"));
            emit failed(rest);
        }
    }
}

void VoiceInput::setState(const QString &state)
{
    if (m_state == state)
        return;
    m_state = state;
    if (state != QStringLiteral("listening")) {
        m_level = 0;
        emit levelChanged();
    }
    emit stateChanged();
}

void VoiceInput::setReady(bool ready)
{
    if (m_ready == ready)
        return;
    m_ready = ready;
    emit readyChanged();
}
