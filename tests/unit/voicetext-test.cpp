// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// VoiceText: the helper's numbered phrases, undo, selection, replacement.

#include "voicetext.h"

#include <cstdio>

namespace {

int failures = 0;

void check(bool ok, const char *what, const QString &got = QString())
{
    if (!ok) {
        ++failures;
        std::printf("FAIL %s%s%s\n", what, got.isEmpty() ? "" : ": ", qPrintable(got));
    }
}

void expect(const VoiceText &t, const char *fixed, const char *open, const char *what)
{
    check(t.fixedText() == QString::fromUtf8(fixed) && t.openText() == QString::fromUtf8(open), what,
          QStringLiteral("[%1] [%2]").arg(t.fixedText(), t.openText()));
}

void phrases()
{
    VoiceText t;
    t.startRecording();
    t.open(0, QStringLiteral("The meeting is on Tuesday."));
    expect(t, "", "The meeting is on Tuesday.", "first phrase open");
    // The pair job: phrase 0 fixed as decoded with phrase 1, then phrase 1 open.
    t.fixed(0, QStringLiteral("The meeting is on Tuesday at"));
    t.open(1, QStringLiteral("three in the room."));
    expect(t, "The meeting is on Tuesday at", "three in the room.", "pair");
    check(t.undoFrom() == 0 && t.undoTo() == 6, "undo range of phrase 0");
}

void undoOpen()
{
    VoiceText t;
    t.startRecording();
    t.fixed(0, QStringLiteral("One two."));
    t.open(1, QStringLiteral("Three."));
    check(t.undo(), "undo open");
    expect(t, "One two.", "", "open removed");
    // Phrase 1 comes back as fixed with the next pair: it stays removed.
    t.fixed(1, QStringLiteral("Three four."));
    t.open(2, QStringLiteral("Five."));
    expect(t, "One two.", "Five.", "undone phrase ignored");
    check(t.undo() && t.undo(), "undo open and phrase 0");
    expect(t, "", "", "all undone");
    check(!t.undo(), "nothing left to undo");
}

void replace()
{
    VoiceText t;
    t.startRecording();
    t.fixed(0, QStringLiteral("The meeting is on Tuesday at"));
    t.commitOpen();
    t.select(5, 4);
    check(t.selectionStart() == 4 && t.selectionEnd() == 5, "selection ordered");
    t.startRecording();   // continue: phrase 0 replaces the selection
    check(t.replacing(), "replacing");
    t.open(0, QStringLiteral("Wednesday morning."));
    expect(t, "The meeting is on Wednesday morning", "", "replaced at once, period dropped");
    check(t.selectionStart() < 0 && !t.replacing(), "selection ends with the replacement");
    t.fixed(0, QStringLiteral("Wednesday morning."));
    t.open(1, QStringLiteral("And more."));
    expect(t, "The meeting is on Wednesday morning", "And more.", "later decoding ignored");
    check(t.undo(), "undo open");
    check(t.undoRestores() == (QStringList() << QStringLiteral("Tuesday") << QStringLiteral("at")),
          "undo restores the old words", t.undoRestores().join(QLatin1Char(' ')));
    check(t.undo(), "undo replacement");
    expect(t, "The meeting is on Tuesday at", "", "old words back");
}

void deleteAndUndo()
{
    VoiceText t;
    t.startRecording();
    t.fixed(0, QStringLiteral("The meeting is here."));
    t.select(0, 0);
    check(t.deleteSelection(), "delete");
    expect(t, "meeting is here.", "", "deleted");
    check(t.undoFrom() == 0 && t.undoTo() == 0, "a deletion removes nothing on undo");
    check(t.undo(), "undo delete");
    expect(t, "The meeting is here.", "", "restored");
}

void selectionOfOpenWords()
{
    // Selected while the open phrase was shown, fixed by the pause since.
    VoiceText t;
    t.startRecording();
    t.fixed(0, QStringLiteral("One two"));
    t.open(1, QStringLiteral("three four"));
    t.select(3, 9);
    t.commitOpen();
    t.startRecording();
    check(t.selectionStart() == 3 && t.selectionEnd() == 3, "selection clamped to the words");
}

void fit()
{
    const auto f = [](const char *words, const char *old) {
        return VoiceText::fitReplacement(QString::fromUtf8(words).split(QLatin1Char(' ')),
                                         QString::fromUtf8(old).split(QLatin1Char(' ')))
            .join(QLatin1Char(' '));
    };
    check(f("Call.", "meeting") == QStringLiteral("call"), "lowercase, no period", f("Call.", "meeting"));
    check(f("kitchen.", "room,") == QStringLiteral("kitchen,"), "the old punctuation", f("kitchen.", "room,"));
    check(f("garden", "The") == QStringLiteral("Garden"), "capital kept", f("garden", "The"));
    check(f("I'm", "we're") == QStringLiteral("I'm"), "I'm stays", f("I'm", "we're"));
    check(f("NASA.", "it") == QStringLiteral("NASA"), "acronym stays", f("NASA.", "it"));
}

} // namespace

int main()
{
    phrases();
    undoOpen();
    replace();
    deleteAndUndo();
    selectionOfOpenWords();
    fit();
    std::printf(failures ? "%d failures\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
