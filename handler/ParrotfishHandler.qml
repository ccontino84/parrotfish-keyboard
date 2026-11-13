// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LGPL-2.1-or-later

// Parrotfish prediction handler for jolla-keyboard.
//
// Based on PreeditHandler.qml of jolla-keyboard 0.10.4 (Jolla Ltd.,
// BSD-3-Clause, see LICENSES/BSD-3-Clause.txt): the word being typed is
// kept as preedit and committed on Space, punctuation or Return. Changes:
// - suggestions and next words come from our engine (Parrotfish.Keyboard);
// - autocorrect (Normal by default; Gentle, Off): Space or punctuation commits the engine's
//   correction, shown first and highlighted in the bar (the typed word
//   stays in the text field and is not repeated in the bar);
// - Backspace straight after an autocorrection brings back what was typed;
//   a word changed by hand later (Backspace into it) is logged as a manual
//   correction, or as a late undo if it was an autocorrection;
// - Backspace after an emoji of several code points (a skin tone, a flag,
//   a ZWJ sequence such as 🧑‍💻) deletes all of them, not one at a time;
// - punctuation straight after a picked suggestion replaces its space with
//   a Backspace key event (also in Android apps); ! ? : ; keep it if the
//   user wants a space before them (dconf
//   /apps/parrotfish-keyboard/space-before-punctuation, as in French: mot ?);
// - a letter typed right at the end of a word (no space: the keyboard was
//   closed in the middle of it, or the cursor was put there) goes on with
//   that word;
// - a tap that starts on one letter key and lifts on another types the
//   letter that makes the more likely word so far, and the engine keeps
//   both keys for that letter: the whole word decides (autocorrect);
// - with the number row, one digit after letters stays in the word, so a
//   slip onto the digit above a letter can be autocorrected (h3llo); a word
//   right after a digit (2nd) is never autocorrected;
// - the real key geometry and the text before the cursor go to the engine;
// - the typing languages come from dconf /apps/parrotfish-keyboard/languages
//   (settings/ParrotfishLanguages.qml);
// - the spacebar shows the typing language;
// - swiping to another layout can be switched off (dconf
//   /apps/parrotfish-keyboard/swipe: normal, off); a long press on the
//   spacebar still opens Jolla's menu of layouts;
// - an empty field leans towards the language the last text was in
//   (dconf /apps/parrotfish-keyboard/last-language);
// - thin dividers between the suggestions; the suggestion
//   delegates are those of HorizontalPredictionListView.qml and
//   VerticalPredictionListView.qml (Jolla Ltd., BSD-3-Clause) plus a divider;
// - an emoji button at the bar's right end: the layout's emoji panel, or
//   the emoji for the emoticon before the cursor (emoticons.js; on Space
//   with /apps/parrotfish-keyboard/replace-emoticons).
import QtQuick 2.6
import com.meego.maliitquick 1.0
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as SilicaPrivate
import Nemo.Configuration 1.0
import com.jolla.keyboard 1.0
import Parrotfish.Keyboard 1.0

InputHandler {
    id: inputHandler

    property var engine: predictionEngine

    readonly property string language: keyboard.language
    readonly property int dividerWidth: Math.max(1, Math.round(Theme.pixelRatio))
    property string preedit

    readonly property bool showApplicationSuggestions: (MInputMethodQuick.surroundingText.length + preedit.length) < 1
                                                       && suggestionModel.count > 0
    readonly property string combinedText: {
        if (showApplicationSuggestions) {
            var surrounding = MInputMethodQuick.surroundingText
            var position = MInputMethodQuick.cursorPosition
            return surrounding.substr(0, position) + preedit + surrounding.substr(position)
        }

        return ""
    }

    // What a picked suggestion committed, with the space it added, while
    // punctuation can still replace that space; "" otherwise. Also the text
    // before the cursor at the pick: Android apps can report the new text
    // late, so unchanged text means the cursor is still after the space.
    property string _autoSpaced: ""
    property string _autoSpacedBefore: ""
    // That space came after a sign that replaced the pick's (word. ): a
    // next . ? ! goes before it (word... word?!), never after a space.
    property bool _autoSpacedSign: false
    // The last autocorrection, while Backspace can still undo it:
    // { typed, corrected, suffix }.
    property var _correction: null
    property string _lastPreedit: ""
    // True while commit() runs: an emptied preedit is then a committed word,
    // not a word deleted with Backspace.
    property bool _committing: false
    // The text before the cursor once the app has the text commit() sent
    // (the autocorrection, not the word as typed). Taken before sending:
    // maliit adds a commit to its copy of the field at once, without a
    // signal (not after a selection or a replacement), and the app reports
    // it later; Android apps unverified.
    property string _committedContext: ""
    // The text before the word being typed, from when it started.
    property string _wordContext: ""
    // A commit was sent and the app hasn't reported its text since: the
    // field may not have it yet (Android apps: maliit's copy isn't updated
    // either), so the text before the cursor is _committedContext.
    property bool _awaitingReport: false
    // The field had text (it emptying ends a text, for learning).
    property bool _fieldHadText: false
    property string _loggedContext: ""   // debug log only
    // Whether the layout has digit keys among the letters (the number row):
    // they are in the engine's key map, and a digit can be part of a word.
    property bool _digitKeys: false
    // The word being typed comes right after a digit (2nd): typed as it is.
    property bool _afterDigit: false
    // The last text we committed ends with a digit (the app may not have
    // reported it yet).
    property bool _committedDigit: false
    // The word Backspace went back into ({ word }), until the next word is
    // committed: if that differs, the user corrected it by hand.
    property var _reopened: null
    // The text before a word taken back as the word being typed (Backspace
    // into it, or typing on at its end), for the context: the app still
    // reports the word itself before the cursor. null: the text before.
    property var _contextBefore: null
    // The word being typed when the keyboard last closed (committed as it
    // was), to type on with when the cursor is back at its end.
    property string _interrupted: ""
    // An emoji being deleted ({ rest, before }), or null. Apps delete one
    // code point per Backspace key event (🧑‍💻 is three): the next event is
    // sent once the app shows the last one done.
    property var _emojiDeleting: null
    // An emoticon Space just replaced ({ text, emoji }), for Backspace to
    // bring back straight after; else null.
    property var _emoticonReplaced: null
    // When the last key was a Space (or a picked suggestion added one), for
    // the period on double space; else 0. _periodAdded: that Space just
    // turned "word " into "word. ", for Backspace to bring back.
    property double _spaceTime: 0
    property bool _periodAdded: false
    // For each letter of preedit: "" or the tap's other key and "1"/"0" (the
    // letter shown is where the finger lifted / touched down), for the
    // engine (predictionEngine.preeditKeys). Kept in step with preedit.
    property var _tapKeys: []
    property string _lastTapKey: ""   // the same for the last tap
    // The last autocorrections ({ typed, corrected }, newest last), to tell
    // a late undo from a manual correction.
    property var _recentCorrections: []
    // The last undone autocorrections ({ typed, corrected }, newest last):
    // going back into the restored word and ending it as the correction
    // again (Space, a pick or retyping it) is a redo.
    property var _recentUndos: []
    // The layout whose keys the engine has, and two of its keys (the first
    // and the last letter key: opposite corners) with their geometry when
    // read. If neither moved, the keys are taken as unchanged.
    property Item _probeLayout: null
    property Item _probeFirst: null
    property Item _probeLast: null
    property string _probeFirstAt: ""
    property string _probeLastAt: ""
    // The layout's keysRevision when the keys were read (our layouts bump it
    // when keys change letters in place: the language keys).
    property int _probeRevision: -1
    property Item _spacebar: null
    // The spacebar's text item, switched to styled text so that the language
    // can be shown in italics; null if it can't be found.
    property Item _spacebarLabel: null
    // The keyboard's own drag threshold, restored when we're not active.
    property real _jollaDragThreshold: -1

    PredictionEngine {
        id: predictionEngine
        language: inputHandler.language
        languages: languagesConfig.value
        strength: strengthConfig.value
        learning: learningConfig.value
        // A field can ask not to be learned from (the settings' preview).
        learnPaused: !!MInputMethodQuick.extensions.parrotfishNoLearning
        slipsConfig: {
            var layout = keyboard.layout
            if (!layout || layout.keyHeightPercent === undefined)
                return ""
            return layout.arrangementName.replace(/ /g, "-") + " "
                    + (keyboard.portraitMode ? "portrait" : "landscape")
                    + " height=" + layout.keyHeightPercent
                    + (layout.numberRowOn ? " numbers=" + layout.numberRowPercent : "")
        }
        onLeadingLanguageChanged: inputHandler._updateSpacebar()
        // New parameters reset the engine: pass the keys again.
        onStrengthChanged: inputHandler._probeLayout = null
    }

    ConfigurationValue {
        id: lastLanguageConfig
        key: "/apps/parrotfish-keyboard/last-language"   // en, it, pt_BR...
        defaultValue: ""
    }

    ConfigurationValue {
        id: languagesConfig
        // "en it de": the typing languages; "": the system language
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

    ConfigurationValue {
        id: swipeConfig
        key: "/apps/parrotfish-keyboard/swipe"   // normal, off (anything else: normal)
        defaultValue: "normal"
        onValueChanged: inputHandler._applySwipe()
    }

    ConfigurationValue {
        id: replaceEmoticonsConfig
        key: "/apps/parrotfish-keyboard/replace-emoticons"   // false, true
        defaultValue: false
    }

    ConfigurationValue {
        id: doubleSpacePeriodConfig
        key: "/apps/parrotfish-keyboard/double-space-period"   // true (the default), false
        defaultValue: true
    }

    ConfigurationValue {
        id: spaceBeforeConfig
        key: "/apps/parrotfish-keyboard/space-before-punctuation"   // false, true
        defaultValue: false
    }

    // Bumped by the settings page after it changed learning.db.
    ConfigurationValue {
        id: learningChangedConfig
        key: "/apps/parrotfish-keyboard/learning-changed"
        defaultValue: 0
        property string _seen   // the value last acted on (not a binding)
        Component.onCompleted: _seen = String(value)
        onValueChanged: {
            if (String(value) === _seen)
                return
            _seen = String(value)
            predictionEngine.reloadLearned()
        }
    }

    ConfigurationValue {
        id: learningConfig
        key: "/apps/parrotfish-keyboard/learning"   // on, privacy, off
        defaultValue: "on"
    }

    ConfigurationValue {
        id: strengthConfig
        key: "/apps/parrotfish-keyboard/autocorrect"   // off, gentle, normal, strong
        defaultValue: "normal"
    }

    onSelect: {
        if (showApplicationSuggestions) {
            MInputMethodQuick.sendCommit(text, -MInputMethodQuick.cursorPosition,
                                         MInputMethodQuick.surroundingText.length)
            _awaitingReport = false   // not followed: the field's text
        } else {
            applyPrediction(text, index)
        }
    }

    onRemove: {
        // We're using an unused key modifier flag as a back channel.
        MInputMethodQuick.sendKey(Qt.Key_Delete, 0x80000000, text)
        _awaitingReport = false
    }

    onPaste: {
        inputHandler.commit(inputHandler.preedit)
        MInputMethodQuick.sendCommit(Clipboard.text)
        inputHandler._awaitingReport = false
    }

    onActiveChanged: {
        // Jolla's handler has no emoji button: nothing to go back with.
        if (!active && keyboard.layout && keyboard.layout.emojiView === true)
            keyboard.layout.emojiView = false
        if (!active && preedit !== "") {
            _interrupted = preedit
            commit(preedit)
        }
        if (!active)
            predictionEngine.endText(true)   // the keyboard closed: a text ends, fading is done if due
        else
            predictionEngine.opened()        // fading still due if the keyboard didn't close cleanly
        _correction = null
        _reopened = null
        _recentCorrections = []
        _recentUndos = []
        _applySwipe()
        if (active) {
            _updateLayout()
            _contextChanged("open")
        }
    }

    onPreeditChanged: {
        if (preedit.length > 0 && _lastPreedit.length === 0) {
            // A new word: refresh the keys and the language context first.
            _updateLayout()
            _wordContext = _contextBefore !== null ? _contextBefore : _currentTextBefore()
            _setContext(_wordContext, "word")
            _contextBefore = null
        } else if (preedit.length === 0 && _lastPreedit.length > 0 && _committing) {
            // The word was committed. The app reports the new text a moment
            // later; until then, assume what was sent.
            _setContext(_committedContext, "commit")
            _rememberLanguage()
            predictionEngine.countWord()
        } else if (preedit.length === 0 && _lastPreedit.length > 0) {
            // The word was deleted letter by letter: only the text before it
            // counts (its last letter, say "i", is not a word typed).
            _setContext(_textBefore(), "deleted")
        }
        _lastPreedit = preedit
        var keys = _tapKeys.slice(0, preedit.length)
        while (keys.length < preedit.length)
            keys.push("")
        _tapKeys = keys
        predictionEngine.preeditKeys = keys
        predictionEngine.preedit = preedit
    }

    Connections {
        target: MInputMethodQuick
        onSurroundingTextChanged: {
            inputHandler._awaitingReport = false
            // The field emptied (a message sent): a text ends.
            var empty = MInputMethodQuick.surroundingTextValid && MInputMethodQuick.surroundingText === ""
            if (empty && inputHandler._fieldHadText)
                predictionEngine.endText()
            inputHandler._fieldHadText = !empty
            inputHandler._contextChanged("text")
            if (inputHandler._emojiDeleting !== null)
                emojiDeletingCheck.restart()
        }
        onCursorPositionChanged: {
            inputHandler._awaitingReport = false
            inputHandler._contextChanged("cursor")
            if (inputHandler._emojiDeleting !== null)
                emojiDeletingCheck.restart()
        }
        // Debug log only: what the field tells, and the keyboard's handler then.
        onHiddenTextChanged: inputHandler._logField("hidden text")
        onContentTypeChanged: inputHandler._logField("content type")
        onPredictionEnabledChanged: inputHandler._logField("prediction enabled")
        onFocusTargetChanged: inputHandler._logField("focus target, active editor " + activeEditor)
    }

    // The keyboard's own handlers react to the same signals: log after them.
    Timer {
        id: fieldLog
        interval: 0
        property string why
        onTriggered: {
            var layoutHandler = keyboard.layout ? keyboard.layout.handler : null
            predictionEngine.log("field (" + why + "): hidden " + MInputMethodQuick.hiddenText
                                 + ", content type " + MInputMethodQuick.contentType
                                 + ", prediction " + MInputMethodQuick.predictionEnabled
                                 + ", layout changes allowed " + keyboard.allowLayoutChanges
                                 + ", keyboard handler is " + (keyboard.inputHandler === inputHandler ? "ours" : "another")
                                 + ", layout wants " + (layoutHandler === inputHandler ? "ours" : "another")
                                 + ", active " + active)
        }
    }

    function _logField(why) {
        if (!predictionEngine.debug)
            return
        fieldLog.why = fieldLog.running ? fieldLog.why + ", " + why : why
        fieldLog.restart()
    }

    function formatText(text) {
        if (text === undefined) {
            return ""
        } else if (showApplicationSuggestions) {
            return Theme.highlightText(text, combinedText, palette.highlightColor)
        }

        if (preedit !== "" && text === predictionEngine.autocorrection) {
            // What Space will type: highlighted as a whole.
            return "<b><font color=\"" + palette.highlightColor + "\">" + _escape(text) + "</font></b>"
        }

        var preeditLength = preedit.length
        if (preeditLength > 0 && text.substr(0, preeditLength) === preedit) {
            return "<font color=\"" + palette.highlightColor + "\">" + _escape(preedit) + "</font>"
                    + _escape(text.substr(preeditLength))
        }

        return _escape(text)
    }

    // The emoticon right before the cursor ({ text, emoji }), or null; the
    // word being typed counts (:P). The text before that word is the one
    // from when it started: some apps report the word in their text too.
    readonly property var emoticonBefore: Emoticons.atEnd(_textBeforeWord() + preedit)

    function _textBeforeWord() {
        return preedit !== "" ? _wordContext : _currentTextBefore()
    }

    // The emoticon is deleted with Backspace key events (every app handles
    // those, unlike replacing text), then the emoji typed.
    function replaceEmoticon() {
        var found = emoticonBefore
        if (!found)
            return
        var typed = preedit
        var before = _textBeforeWord()
        var length = found.text.length - typed.length   // what was committed of it
        if (typed !== "") {
            preedit = ""
            MInputMethodQuick.sendPreedit("")
        }
        for (var n = 0; n < length; ++n)
            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
        _committedContext = before.substring(0, before.length - length)
        _awaitingReport = true
        commit(found.emoji)
        _emojiUsed(found.emoji)
    }

    function applyPrediction(replacement, index) {
        var typed = preedit
        var picked = typed !== "" && replacement !== typed
        if (picked)
            predictionEngine.acceptCorrection(typed)   // forgets earlier undos
        if (!_checkManual(replacement, true) && picked)
            predictionEngine.logSlips(typed, replacement, "picked")
        predictionEngine.countUse(typed === "" ? "next" : "picked", replacement)
        replacement = replacement + " "
        var before = _textBefore()
        commit(replacement)
        _autoSpaced = replacement
        _autoSpacedBefore = before
        _autoSpacedSign = false
        // As Space: a next word (nothing was typed, so the preedit's commit
        // path didn't run) counts as a committed word too, and Shift goes
        // back to automatic (Jolla does that too, but only when the app
        // reports the cursor).
        if (typed === "") {
            _rememberLanguage()
            predictionEngine.countWord()
        }
        if (keyboard.shiftState !== ShiftState.LockedShift)
            keyboard.shiftState = ShiftState.AutoShift
        _spaceTime = Date.now()
    }

    function handleKeyClick() {
        var handled = false
        var correction = _correction
        _correction = null   // Backspace can only undo straight after
        _emojiDeleting = null
        var emoticonReplaced = _emoticonReplaced
        _emoticonReplaced = null
        var spaceTime = _spaceTime
        _spaceTime = 0
        var periodAdded = _periodAdded
        _periodAdded = false
        if (keyboard.layout && keyboard.layout.emojiView === true && pressedKey.text !== ""
                && (pressedKey.keyType === KeyType.CharacterKey || pressedKey.keyType === KeyType.PopupKey))
            _emojiUsed(pressedKey.text)
        if (correction !== null && !(pressedKey.key === Qt.Key_Backspace && preedit === "")) {
            predictionEngine.acceptCorrection(correction.typed)   // kept
            if (!correction.redone)   // already logged as redone
                predictionEngine.logSlips(correction.typed, correction.corrected, "kept")
        }
        var autoSpaced = _autoSpaced
        if (pressedKey.keyType !== KeyType.ShiftKey && pressedKey.keyType !== KeyType.SymbolKey)
            _autoSpaced = ""   // only the next key (after Shift or ?123) may replace it
        keyboard.expandedPaste = false

        if (pressedKey.key === Qt.Key_Space) {
            var emoticon = emoticonBefore
            if (emoticon !== null && (replaceEmoticonsConfig.value === true
                                      || replaceEmoticonsConfig.value === "true")) {
                replaceEmoticon()
                commit(" ")
                _emoticonReplaced = emoticon
            } else if (preedit !== "") {
                _commitWord(" ")
                keyboard.autocaps = false // assuming no autocaps after input
            } else if (_periodOnDoubleSpace(spaceTime)) {
                // "word " → "word. ": the space deleted with a Backspace key
                // event (as for a picked suggestion's), then ". ".
                var spaced = _currentTextBefore()
                MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
                _committedContext = spaced.slice(0, -1)
                _awaitingReport = true
                commit(". ")
                _periodAdded = true
            } else {
                commit(" ")
            }

            if (keyboard.shiftState !== ShiftState.LockedShift) {
                keyboard.shiftState = ShiftState.AutoShift
            }
            if (!_periodAdded)
                _spaceTime = Date.now()

            handled = true

        } else if (pressedKey.key === Qt.Key_Return) {
            if (preedit !== "") {
                _checkManual(preedit)
                commit(preedit)
            }
            if (keyboard.shiftState !== ShiftState.LockedShift) {
                keyboard.shiftState = ShiftState.AutoShift
            }

        } else if (pressedKey.key === Qt.Key_Backspace && preedit === "" && correction !== null
                   && _canUndo(correction)) {
            // Undo the autocorrection, keeping the space or punctuation: delete
            // the corrected word with Backspace key events (every app handles
            // those, unlike replacing text) and type back what was typed.
            var length = correction.corrected.length + correction.suffix.length
            for (var n = 0; n < length; ++n)
                MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            MInputMethodQuick.sendCommit(correction.typed + correction.suffix)
            _awaitingReport = false
            predictionEngine.replaceKept(correction.corrected, correction.typed)
            _recentUndos = _recentUndos.slice(-4).concat([{ "typed": correction.typed,
                                                            "corrected": correction.corrected }])
            predictionEngine.rejectCorrection(correction.typed)
            predictionEngine.logSlips(correction.typed, correction.corrected, "undone")
            predictionEngine.log("undo: " + length + " characters replaced by "
                                 + (correction.typed.length + correction.suffix.length))
            handled = true

        } else if (pressedKey.key === Qt.Key_Backspace && preedit === "" && emoticonReplaced !== null
                   && _currentTextBefore().slice(-emoticonReplaced.emoji.length - 1) === emoticonReplaced.emoji + " ") {
            // Undo the emoticon's replacement: delete the emoji (a key event
            // per code point) and the space, type back the emoticon.
            var points = EmojiUtil.codePoints(emoticonReplaced.emoji).split("-").length + 1
            for (var k = 0; k < points; ++k)
                MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            commit(emoticonReplaced.text + " ")
            handled = true

        } else if (pressedKey.key === Qt.Key_Backspace && preedit === "" && periodAdded
                   && _currentTextBefore().slice(-2) === ". ") {
            // Undo the period on double space: back to the one space.
            var dotted = _currentTextBefore()
            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            _committedContext = dotted.slice(0, -2)
            _awaitingReport = true
            commit(" ")
            if (keyboard.shiftState !== ShiftState.LockedShift)
                keyboard.shiftState = ShiftState.AutoShift
            handled = true

        } else if (pressedKey.key === Qt.Key_Backspace && preedit !== "") {
            preedit = preedit.substr(0, preedit.length - 1)
            MInputMethodQuick.sendPreedit(preedit)

            if (keyboard.shiftState !== ShiftState.LockedShift) {
                if (preedit.length === 0) {
                    keyboard.shiftState = ShiftState.AutoShift
                } else {
                    keyboard.shiftState = ShiftState.NoShift
                }
            }

            handled = true

        } else if (pressedKey.text.length !== 0) {
            if (isInputCharacter(pressedKey.text) || _digitInWord(pressedKey.text)) {
                var resumed = 0
                if (preedit === "") {
                    var before = _wordAtCursor()
                    resumed = before.length
                    if (resumed > 0) {
                        predictionEngine.dropKept()   // the word goes on
                        _interrupted = ""
                        var text = MInputMethodQuick.surroundingText
                        var start = MInputMethodQuick.cursorPosition - resumed
                        _afterDigit = _isDigit(text.charAt(start - 1))
                        _contextBefore = text.substring(Math.max(0, start - 1000), start)
                        preedit = before
                    } else {
                        _afterDigit = _committedDigit || _isDigit(_textBefore().slice(-1))
                    }
                }
                var tapped = _tappedText()
                var keys = _tapKeys.slice(0, preedit.length).concat([_lastTapKey])
                while (keys.length < preedit.length + tapped.length)
                    keys.push("")
                _tapKeys = keys
                preedit += tapped

                if (keyboard.shiftState !== ShiftState.LockedShift) {
                    keyboard.shiftState = ShiftState.NoShift
                }

                if (resumed > 0)
                    MInputMethodQuick.sendPreedit(preedit, undefined, -resumed, resumed)
                else
                    MInputMethodQuick.sendPreedit(preedit)
                handled = true
            } else {
                // normal symbols etc.
                if (preedit !== "") {
                    if (".,;:!?)\"".indexOf(pressedKey.text) >= 0) {
                        _commitWord(pressedKey.text)
                    } else {
                        _checkManual(preedit)
                        commit(preedit + pressedKey.text)
                    }
                } else {
                    var afterSign = _autoSpacedSign
                    if (autoSpaced !== "" && (afterSign ? ".?!" : ",.;:?!").indexOf(pressedKey.text) >= 0
                            && (_endsTextBefore(autoSpaced) || _textBefore() === _autoSpacedBefore)) {
                        // Replace the space added by a picked suggestion: a
                        // Backspace key event, as for undo (Jolla's replacing
                        // commit failed in Android apps). Kept before ! ? : ;
                        // if the user wants it (mot ?), not between signs.
                        var reported = _textBefore()   // as at a pick: unchanged if the app is late
                        if (afterSign || !_spaceBefore(pressedKey.text)) {
                            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
                            if (_awaitingReport)
                                _committedContext = _committedContext.slice(0, -1)
                        }
                        commit(pressedKey.text + " ")
                        if (".?!".indexOf(pressedKey.text) >= 0) {
                            _autoSpaced = pressedKey.text + " "
                            _autoSpacedBefore = reported
                            _autoSpacedSign = true
                        }
                    } else {
                        commit(pressedKey.text)
                    }
                }
                handled = true
            }
        } else if (pressedKey.key === Qt.Key_Backspace
                   && MInputMethodQuick.surroundingTextValid
                   && !MInputMethodQuick.hasSelection
                   && MInputMethodQuick.cursorPosition >= 2
                   && isInputCharacter(MInputMethodQuick.surroundingText.charAt(MInputMethodQuick.cursorPosition - 2))) {
            // backspacing into a word, re-activate it
            var wordLength = 1
            var pos = MInputMethodQuick.cursorPosition - 3
            for (; pos >= 0 && isInputCharacter(MInputMethodQuick.surroundingText.charAt(pos)); --pos) {
                wordLength++
            }
            pos++

            var word = MInputMethodQuick.surroundingText.substring(pos, pos + wordLength)
            var deleted = MInputMethodQuick.surroundingText.charAt(MInputMethodQuick.cursorPosition - 1)
            _reopened = { "word": word + (isInputCharacter(deleted) ? deleted : "") }
            _afterDigit = _isDigit(MInputMethodQuick.surroundingText.charAt(pos - 1))
            _contextBefore = MInputMethodQuick.surroundingText.substring(Math.max(0, pos - 1000), pos)
            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            MInputMethodQuick.sendPreedit(word, undefined, -wordLength, wordLength)
            _awaitingReport = false
            predictionEngine.dropKept()   // being typed again
            preedit = word
            handled = true

        } else if (pressedKey.key === Qt.Key_Backspace && _deleteEmoji()) {
            handled = true
        }

        return handled
    }

    // An emoji typed: the layout's recent emoji have it, except in the
    // settings' preview field, which keeps nothing (parrotfishNoLearning).
    function _emojiUsed(emoji) {
        if (keyboard.layout && typeof keyboard.layout.emojiUsed === "function"
                && !MInputMethodQuick.extensions.parrotfishNoLearning)
            keyboard.layout.emojiUsed(emoji)
    }

    // Backspace after an emoji of several code points: deletes all of them
    // (_emojiDeleting goes on when the app reports each one gone).
    function _deleteEmoji() {
        if (!MInputMethodQuick.surroundingTextValid || MInputMethodQuick.hasSelection)
            return false
        var before = _textBefore()
        var emoji = EmojiUtil.emojiAtEnd(before)
        if (emoji === "")
            return false
        _emojiDeleting = { "rest": EmojiUtil.withoutLastCodePoint(emoji),
                           "before": EmojiUtil.withoutLastCodePoint(before) }
        emojiDeletingTimeout.restart()
        MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
        return true
    }

    function _emojiDeletingNext() {
        var deleting = _emojiDeleting
        if (deleting === null)
            return
        var now = _textBefore()
        var expected = deleting.before
        // _textBefore() is a window of the last 1000 characters: in a longer
        // text it moves back as text is deleted, so only its end is compared.
        if (now.length >= 900 && expected.length >= 900) {
            now = now.substring(now.length - 100)
            expected = expected.substring(expected.length - 100)
        }
        if (now !== expected)
            return   // not yet, or the app did something else: stop at the timeout
        if (deleting.rest === "") {
            _emojiDeleting = null
            return
        }
        _emojiDeleting = { "rest": EmojiUtil.withoutLastCodePoint(deleting.rest),
                           "before": EmojiUtil.withoutLastCodePoint(deleting.before) }
        emojiDeletingTimeout.restart()
        MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
    }

    // The text and the cursor are reported one after the other: checked
    // once both are in.
    Timer {
        id: emojiDeletingCheck
        interval: 0
        onTriggered: inputHandler._emojiDeletingNext()
    }

    Timer {
        id: emojiDeletingTimeout
        interval: 1000
        onTriggered: inputHandler._emojiDeleting = null
    }

    // The word the cursor is right at the end of (word characters before it,
    // none after it, at least one letter), to type on with; "" if none. Inside
    // a word, nothing: typing there inserts. Android apps report only the
    // text before the cursor, so where the app reports nothing after it, the
    // cursor may be inside a word: then only the word interrupted when the
    // keyboard closed is taken back, if the text still ends with it.
    function _wordAtCursor() {
        if (!MInputMethodQuick.surroundingTextValid || MInputMethodQuick.hasSelection)
            return ""
        var text = MInputMethodQuick.surroundingText
        var cursor = MInputMethodQuick.cursorPosition
        if (cursor >= text.length) {
            var interrupted = _interrupted
            if (interrupted === "" || text.substr(cursor - interrupted.length, interrupted.length) !== interrupted
                    || isInputCharacter(text.charAt(cursor - interrupted.length - 1)))
                return ""
        } else if (isInputCharacter(text.charAt(cursor))) {
            return ""
        }
        var pos = cursor
        while (pos > 0 && cursor - pos < 40 && isInputCharacter(text.charAt(pos - 1)))
            --pos
        if (pos > 0 && isInputCharacter(text.charAt(pos - 1)))
            return ""   // too long to be a word
        var word = text.substring(pos, cursor)
        for (var i = 0; i < word.length; ++i)
            if (predictionEngine.isLetter(word.charAt(i)))
                return word
        return ""
    }

    function isInputCharacter(character) {
        return character !== "" && (predictionEngine.isLetter(character) || "\'-".indexOf(character) >= 0)
    }

    function reset() {
        preedit = ""
        _correction = null
        _reopened = null
        _committedDigit = false
    }

    function commit(text) {
        var hadWord = preedit !== ""
        if (!hadWord)
            _reopened = null   // the word went back into was deleted and not retyped
        _committing = true
        // The text before the word being typed: some apps show it in the
        // field they report (the field grows while typing), others don't.
        var base = preedit !== "" ? _wordContext : _currentTextBefore()
        _committedContext = (base + text).slice(-1000)   // as _textBefore()
        predictionEngine.committed(base, text)   // learned once kept
        _awaitingReport = true
        MInputMethodQuick.sendCommit(text)
        _committedDigit = _isDigit(text.slice(-1))
        preedit = ""
        _committing = false
        // A word's commit sets the context when the preedit empties
        // (onPreeditChanged); a sign or a space alone, here.
        if (!hadWord && active)
            _setContext(_committedContext, "commit")
    }

    // Commits the word being typed, or its autocorrection, then `suffix`.
    function _commitWord(suffix) {
        predictionEngine.flush()
        var typed = preedit
        var word = predictionEngine.autocorrection !== "" && !_afterDigit ? predictionEngine.autocorrection : typed
        // A single letter (English i → I) is fixed at Space only: i. (i.e.)
        // and i) (a list) stay.
        if (typed.length === 1 && suffix !== " ")
            word = typed
        // Back in an autocorrected word, the user typed it as before: they
        // meant it (a late undo), don't correct it again.
        if (word !== typed && _reopened !== null && _recentCorrection(_reopened.word, typed))
            word = typed
        var redone = _checkManual(word)
        commit(word + suffix)
        if (word !== typed) {
            predictionEngine.countUse("fixed", word)
            _correction = { "typed": typed, "corrected": word, "suffix": suffix, "redone": redone }
            _recentCorrections = _recentCorrections.slice(-4).concat([{ "typed": typed, "corrected": word }])
        }
    }

    // Whether `typed` was recently autocorrected into `corrected`.
    function _recentCorrection(corrected, typed) {
        for (var i = 0; i < _recentCorrections.length; ++i) {
            var c = _recentCorrections[i]
            if (c.corrected.toLowerCase() === corrected.toLowerCase()
                    && c.typed.toLowerCase() === typed.toLowerCase())
                return true
        }
        return false
    }

    // A word is committed: if Backspace went back into a word and this one
    // differs, the user corrected that word by hand. An autocorrection
    // changed that way was wrong: a late undo (twice stops it, as Backspace
    // right after it). `picked`: the word came from the bar, logged as picked
    // unless this was a redo. A word whose autocorrection was undone, ended
    // as that correction again, is a redo: logged as redone, its undos
    // forgotten. Returns whether it was a redo.
    function _checkManual(word, picked) {
        var reopened = _reopened
        _reopened = null
        if (reopened === null || word.toLowerCase() === reopened.word.toLowerCase())
            return false
        for (var u = _recentUndos.length - 1; u >= 0; --u) {
            var undo = _recentUndos[u]
            if (undo.typed.toLowerCase() === reopened.word.toLowerCase()
                    && undo.corrected.toLowerCase() === word.toLowerCase()) {
                _recentUndos = _recentUndos.slice(0, u).concat(_recentUndos.slice(u + 1))
                predictionEngine.acceptCorrection(undo.typed)
                predictionEngine.logSlips(undo.typed, undo.corrected, "redone")
                return true
            }
        }
        var corrections = _recentCorrections
        for (var i = corrections.length - 1; i >= 0; --i) {
            var c = corrections[i]
            if (c.corrected.toLowerCase() === reopened.word.toLowerCase()) {
                _recentCorrections = corrections.slice(0, i).concat(corrections.slice(i + 1))
                predictionEngine.rejectCorrection(c.typed)
                predictionEngine.logSlips(c.typed, c.corrected, "undone late")
                if (!picked && word.toLowerCase() !== c.typed.toLowerCase())
                    predictionEngine.logSlips(c.typed, word, "manual")   // what was meant
                return false
            }
        }
        if (!picked)
            predictionEngine.logSlips(reopened.word, word, "manual")
        return false
    }

    // Period on double space (/apps/parrotfish-keyboard/double-space-period,
    // on unless false): a Space within a second of the one that ended a word
    // (a letter or a digit before it, not punctuation) puts a period before
    // that space; Shift goes to automatic as after any sentence end.
    function _periodOnDoubleSpace(spaceTime) {
        if (spaceTime === 0 || Date.now() - spaceTime > 1000
                || doubleSpacePeriodConfig.value === false || doubleSpacePeriodConfig.value === "false")
            return false
        var before = _currentTextBefore()
        var last = before.charAt(before.length - 2)
        return before.slice(-1) === " " && (predictionEngine.isLetter(last) || _isDigit(last))
    }

    // Whether the sign is written after a space: ! ? : ; if the user wants
    // it (French writes mot ?), whatever the language.
    function _spaceBefore(sign) {
        return "!?:;".indexOf(sign) >= 0
                && (spaceBeforeConfig.value === true || spaceBeforeConfig.value === "true")
    }

    function _isDigit(character) {
        return character.length === 1 && character >= "0" && character <= "9"
    }

    // With the number row, a digit typed after a letter stays in the word
    // (only one: mp34 and h33llo are typed as they are). The engine fixes
    // it only as a slip onto the digit (h3llo → hello, not ps4 → is).
    function _digitInWord(character) {
        return _digitKeys && _isDigit(character) && preedit !== ""
                && predictionEngine.isLetter(preedit.slice(-1)) && !/[0-9]/.test(preedit)
    }

    // Only if the correction is right before the cursor (the cursor may have
    // been moved by tapping the text, which is not a key click).
    function _canUndo(correction) {
        return _endsTextBefore(correction.corrected + correction.suffix)
    }

    // Whether the text right before the cursor is `text`. True when the app
    // doesn't tell: the key came straight after the commit.
    function _endsTextBefore(text) {
        if (!MInputMethodQuick.surroundingTextValid)
            return true
        var before = _textBefore()
        return before.length >= text.length
                && before.substr(before.length - text.length) === text
    }

    // The text of the tapped key; for a tap that started on a neighbouring
    // letter key and slid onto this one, the letter that fits the word better.
    function _tappedText() {
        var text = pressedKey.text
        var initial = keyboard.lastInitialKey
        _lastTapKey = ""
        // lastInitialKey belongs to the latest touch. When a second finger lands
        // before the first lifts, Jolla's keyboard types the first finger's key
        // while lastInitialKey is already the second finger's: then
        // lastPressedKey is the second finger's key too, not the key being typed.
        if (initial && initial !== pressedKey && keyboard.lastPressedKey === pressedKey
                && initial.keyType === pressedKey.keyType
                && typeof initial.text === "string" && initial.text.length === 1
                && text.length === 1 && isInputCharacter(initial.text)
                && _adjacent(initial, pressedKey)) {
            var resolved = predictionEngine.resolveTap(preedit, text, initial.text)
            _lastTapKey = resolved === text ? initial.text + "1" : text + "0"
            text = resolved
        }
        return text
    }

    // Whether two keys are neighbours (a finger can roll from one to the other).
    function _adjacent(a, b) {
        var pa = a.mapToItem(keyboard, a.width / 2, a.height / 2)
        var pb = b.mapToItem(keyboard, b.width / 2, b.height / 2)
        var dx = (pa.x - pb.x) / b.width
        var dy = (pa.y - pb.y) / b.height
        return dx * dx + dy * dy <= 1.6 * 1.6
    }

    function _escape(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    function _contextChanged(why) {
        if (active && preedit.length === 0)
            _setContext(_textBefore(), why)
    }

    // `why`: what changed (debug log only).
    function _setContext(text, why) {
        if (predictionEngine.debug) {
            // Lengths only, never the text.
            var last = _loggedContext, same = 0
            var n = Math.min(text.length, last.length)
            while (same < n && text.charCodeAt(same) === last.charCodeAt(same))
                ++same
            predictionEngine.log("context from " + why + ": " + text.length + " characters"
                                 + (text === last ? ", unchanged" : ", was " + last.length + ", first " + same + " the same")
                                 + "; field " + (MInputMethodQuick.surroundingTextValid ? MInputMethodQuick.surroundingText.length : -1)
                                 + ", cursor " + MInputMethodQuick.cursorPosition)
            _loggedContext = text
        }
        predictionEngine.rememberedLanguage = lastLanguageConfig.value
        predictionEngine.setContext(text)
    }

    // The text before the cursor, or, until the app reports the last
    // commit, what it will be.
    function _currentTextBefore() {
        return _awaitingReport ? _committedContext : _textBefore()
    }

    function _textBefore() {
        if (!MInputMethodQuick.surroundingTextValid)
            return ""
        var pos = MInputMethodQuick.cursorPosition
        return MInputMethodQuick.surroundingText.substring(Math.max(0, pos - 1000), pos)
    }

    // Letter keys and the apostrophe key: the keys words are typed with. The
    // sticky key (the layout's apostrophe key) counts as the apostrophe
    // whatever sign it types, so that the engine's key map stays the same.
    // The number row's digits too: slips onto them are autocorrected.
    function _isLetterKey(item) {
        return item.keyType !== undefined && typeof item.caption === "string"
                && item.caption.length === 1
                && (item.caption.toLowerCase() !== item.caption.toUpperCase() || item.caption === "'"
                    || item.sticky === true || _isDigit(item.caption))
                && item.visible && item.width > 0
    }

    function _scan(item, root, keys, items) {
        var children = item.children
        if (!children)
            return
        for (var i = 0; i < children.length; ++i) {
            var child = children[i]
            if (_isLetterKey(child)) {
                var p = child.mapToItem(root, child.width / 2, child.height / 2)
                keys.push({ "ch": child.sticky === true ? "'" : child.caption, "x": p.x, "y": p.y,
                            "width": child.width, "height": child.height })
                items.push(child)
            } else if (child.key === Qt.Key_Space && child.languageLabel !== undefined
                       && child.languageLabel !== "" && _spacebar === null) {
                _spacebar = child
                _spacebarLabel = _findLabel(child)
            } else {
                _scan(child, root, keys, items)
            }
        }
    }

    // The text item that shows the spacebar's languageLabel.
    function _findLabel(spacebar) {
        for (var i = 0; i < spacebar.children.length; ++i) {
            var c = spacebar.children[i]
            if (c.textFormat !== undefined && c.text === spacebar.languageLabel) {
                c.textFormat = Text.StyledText
                return c
            }
        }
        return null
    }

    // Where a key is, relative to the layout, and its size.
    function _geometry(key, layout) {
        if (!key || !key.visible)
            return ""
        var p = key.mapToItem(layout, 0, 0)
        return p.x + "," + p.y + "," + key.width + "," + key.height
    }

    // Checks the keys at every new word: a geometry read once and kept could
    // be stale (wrong corrections until a settings change made the handler
    // read the keys again). Reading them all costs 2-3 ms on the
    // phone, so only two keys are checked; all are read again if either moved
    // or is gone (rebuilt keys). The engine is rebuilt only when keys change.
    function _updateLayout() {
        var layout = keyboard.layout
        if (!layout)
            return
        var started = Date.now()
        if (layout === _probeLayout && _probeFirstAt !== ""
                && _geometry(_probeFirst, layout) === _probeFirstAt
                && _geometry(_probeLast, layout) === _probeLastAt
                && (layout.keysRevision === undefined || layout.keysRevision === _probeRevision)) {
            predictionEngine.log("layout unchanged, checked in " + (Date.now() - started) + " ms")
            return
        }
        var keys = []
        var items = []
        _spacebar = null
        _spacebarLabel = null
        try {
            _scan(layout, layout, keys, items)
        } catch (e) {
            predictionEngine.log("scanning the layout failed: " + e)
        }
        predictionEngine.log("layout " + layout.languageCode + ": " + keys.length
                             + " letter keys in " + (Date.now() - started) + " ms, spacebar "
                             + (_spacebar !== null) + ", spacebar label " + (_spacebarLabel !== null))
        _digitKeys = false
        for (var i = 0; i < keys.length; ++i)
            if (_isDigit(keys[i].ch))
                _digitKeys = true
        if (predictionEngine.setKeys(keys) > 0) {
            _probeLayout = layout
            _probeFirst = items[0]
            _probeLast = items[items.length - 1]
            _probeFirstAt = _geometry(_probeFirst, layout)
            _probeLastAt = _geometry(_probeLast, layout)
            _probeRevision = layout.keysRevision !== undefined ? layout.keysRevision : -1
        } else {
            _probeLayout = null
        }
        _updateSpacebar()
    }

    // Store the language the text is in, once the engine is fairly sure
    // (written only when it changes). Only after a committed word: while text
    // is deleted, word fragments can look like another language.
    function _rememberLanguage() {
        var lead = predictionEngine.leadingLanguage
        if (lead !== "" && predictionEngine.confidence >= 0.7 && lead !== lastLanguageConfig.value)
            lastLanguageConfig.value = lead
    }

    // A horizontal drag longer than keyboard.dragThreshold switches the
    // layout. With swiping off, while typing on our layouts, ask for one
    // longer than the keyboard; give the keyboard its own value back when
    // another handler takes over.
    function _applySwipe() {
        if (_jollaDragThreshold < 0)
            _jollaDragThreshold = keyboard.dragThreshold
        var threshold = _jollaDragThreshold
        if (active && swipeConfig.value === "off")
            threshold = Math.max(keyboard.width, keyboard.height)
        if (keyboard.dragThreshold !== threshold) {
            keyboard.dragThreshold = threshold
            predictionEngine.log("swipe " + (active ? swipeConfig.value : "inactive")
                                 + ": drag threshold " + Math.round(threshold)
                                 + " (keyboard's own " + Math.round(_jollaDragThreshold) + ")")
        }
    }

    // The spacebar shows the typing language ("it"; the key arrangement,
    // "QWERTY", until the engine has one). The layout only gives the keys, so
    // it has no language of its own to show. While the engine is unsure, the
    // language is in italics: the handler switches Jolla's label to styled
    // text (it is plain text by default); without the label, no italics.
    function _updateSpacebar() {
        if (!_spacebar || !keyboard.layout)
            return
        var lead = predictionEngine.leadingLanguage
        var unsure = predictionEngine.confidence < 0.7
        var label = typeof keyboard.layout.arrangementName === "string"
                ? keyboard.layout.arrangementName : keyboard.layout.languageCode
        if (lead !== "") {
            lead = lead.split("_")[0]   // pt_BR: pt
            label = unsure && _spacebarLabel ? "<i>" + lead + "</i>" : lead
        }
        if (_spacebar.languageLabel !== label)
            _spacebar.languageLabel = label
    }

    topItem: Component {
        TopItem {
            // The emoji button at the bar's right end, across
            // from Jolla's paste button: it opens the layout's emoji panel
            // (emoji/EmojiPanel.qml), and shows the keyboard icon to go back to the
            // letters. Above the suggestions (z), which are clipped at its edge.
            BackgroundItem {
                id: emojiButton

                readonly property bool open: available && keyboard.layout.emojiView
                // The emoji for the emoticon before the cursor, or "".
                readonly property string offer: !open && inputHandler.emoticonBefore
                                                ? inputHandler.emoticonBefore.emoji : ""
                z: 1

                readonly property bool available: keyboard.layout !== null
                                                  && keyboard.layout.emojiView !== undefined
                anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                width: available ? height : 0
                visible: available
                // With an emoji offered, a tap puts it in the emoticon's place
                // and a long press opens the panel; else both open it.
                onClicked: {
                    if (offer !== "")
                        inputHandler.replaceEmoticon()
                    else
                        keyboard.layout.emojiView = !keyboard.layout.emojiView
                }
                onPressAndHold: keyboard.layout.emojiView = !keyboard.layout.emojiView

                // An offered emoji is highlighted.
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.round(parent.height * 0.8)
                    height: width
                    radius: width / 4
                    color: Theme.highlightBackgroundColor
                    opacity: Theme.highlightBackgroundOpacity
                    visible: emojiButton.offer !== ""
                }

                Icon {
                    anchors.centerIn: parent
                    visible: emojiButton.open
                    source: "image://theme/icon-m-keyboard"
                    highlighted: emojiButton.highlighted
                }

                Image {
                    anchors.centerIn: parent
                    visible: !emojiButton.open
                    readonly property int side: Math.round(parent.height * 0.55)
                    width: side
                    height: side
                    sourceSize.width: side
                    sourceSize.height: side
                    source: !emojiButton.available || side <= 0 ? ""
                            : emojiButton.offer !== "" ? EmojiUtil.source(emojiButton.offer)
                            : EmojiUtil.source("\uD83D\uDE42")
                }
            }

            HorizontalPredictionListView {
                id: predictionList

                anchors.fill: undefined
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom; right: emojiButton.left }
                clip: true
                handler: inputHandler
                model: inputHandler.showApplicationSuggestions ? suggestionModel: predictionEngine
                canRemove: inputHandler.showApplicationSuggestions
                           && !!MInputMethodQuick.extensions.autoFillCanRemove

                // Jolla's delegate plus a divider before every suggestion but
                // the first.
                delegate: BackgroundItem {
                    id: delegate

                    width: Theme.paddingLarge + label.width + predictionList._buttonMargin
                    height: predictionList.height
                    clip: predictionList.canRemove

                    onClicked: {
                        if (predictionList.showRemoveButton) {
                            predictionList.showRemoveButton = false
                        } else {
                            predictionList.handler.select(model.text, model.index)
                        }
                    }

                    onPressAndHold: {
                        if (predictionList.canRemove && !predictionList.showRemoveButton) {
                            predictionList.currentIndex = index
                            delegate.HorizontalAutoScroll.keepVisible = Qt.binding(function() {
                                return delegate.ListView.isCurrentItem
                            })
                            predictionList.showRemoveButton = true
                        } else {
                            predictionList.showRemoveButton = false
                        }
                    }

                    Rectangle {
                        visible: model.index > 0
                        width: inputHandler.dividerWidth
                        height: Math.round(delegate.height / 2)
                        anchors.verticalCenter: parent.verticalCenter
                        color: label.color
                        opacity: 0.3
                    }

                    Label {
                        id: label

                        x: Theme.paddingLarge
                        width: Math.min(implicitWidth, predictionList._maximumLabelWidth + Theme.paddingLarge
                                        - predictionList._buttonMargin)
                        height: delegate.height
                        text: predictionList.handler.formatText(model.text)
                        font.pixelSize: Theme.fontSizeSmall
                        verticalAlignment: Text.AlignVCenter
                        truncationMode: TruncationMode.Fade
                        textFormat: Text.StyledText
                    }

                    IconButton {
                        x: label.x + label.width
                        height: delegate.height
                        width: delegate.height
                        icon.source: "image://theme/icon-m-input-remove"
                        enabled: predictionList.showRemoveButton
                        opacity: predictionList.showRemoveButton ? 1 : 0

                        onClicked: predictionList.handler.remove(model.text, model.index)

                        Behavior on opacity { FadeAnimator { duration: 100 } }
                    }
                }

                Connections {
                    target: predictionEngine
                    onPredictionsChanged: {
                        if (!inputHandler.showApplicationSuggestions) {
                            predictionList.predictionsChanged()
                        }
                    }
                }

                Connections {
                    target: suggestionModel
                    onStringsChanged: {
                        if (inputHandler.showApplicationSuggestions) {
                            predictionList.predictionsChanged()
                        }
                    }
                }
            }
        }
    }

    verticalItem: Component {
        Item {
            id: verticalContainer

            property int inactivePadding: Theme.paddingMedium

            // Empty while the emoji panel is open: the panel is under it.
            VerticalPredictionListView {
                id: verticalList

                visible: !(keyboard.layout && keyboard.layout.emojiView === true)
                handler: inputHandler
                model: inputHandler.showApplicationSuggestions ? suggestionModel: predictionEngine
                canRemove: inputHandler.showApplicationSuggestions
                        && !!MInputMethodQuick.extensions.autoFillCanRemove

                // Jolla's delegate plus a divider above every suggestion but
                // the first.
                delegate: BackgroundItem {
                    id: verticalDelegate

                    width: parent.width
                    height: geometry.keyHeightLandscape // assuming landscape!

                    onClicked: {
                        if (verticalList.showRemoveButton) {
                            verticalList.showRemoveButton = false
                        } else {
                            verticalList.handler.select(model.text, model.index)
                        }
                    }

                    onPressAndHold: {
                        if (verticalList.canRemove && !verticalList.showRemoveButton) {
                            verticalList.currentIndex = index
                            verticalDelegate.HorizontalAutoScroll.keepVisible = Qt.binding(function() {
                                return verticalDelegate.ListView.isCurrentItem
                            })
                            verticalList.showRemoveButton = true
                        } else {
                            verticalList.showRemoveButton = false
                        }
                    }

                    Rectangle {
                        visible: model.index > 0
                        width: Math.round(verticalDelegate.width / 2)
                        height: inputHandler.dividerWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: verticalLabel.color
                        opacity: 0.3
                    }

                    Label {
                        id: verticalLabel

                        width: verticalDelegate.width - verticalList._buttonMargin
                        height: verticalDelegate.height

                        text: verticalList.handler.formatText(model.text)
                        font.pixelSize: Theme.fontSizeSmall

                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter

                        elide: Text.ElideRight
                        textFormat: Text.StyledText
                        fontSizeMode: Text.HorizontalFit
                    }

                    IconButton {
                        x: verticalDelegate.width - width
                        width: Theme.itemSizeExtraSmall
                        height: verticalDelegate.height
                        icon.source: "image://theme/icon-m-input-remove"
                        enabled: verticalList.showRemoveButton
                        opacity: verticalList.showRemoveButton ? 1 : 0

                        onClicked: verticalList.handler.remove(model.text, model.index)

                        Behavior on opacity { FadeAnimator { duration: 100 } }
                    }
                }

                Connections {
                    target: predictionEngine
                    onPredictionsChanged: {
                        if (!inputHandler.showApplicationSuggestions) {
                            verticalList.predictionsChanged()
                        }
                    }
                }

                Connections {
                    target: suggestionModel
                    onStringsChanged: {
                        if (inputHandler.showApplicationSuggestions) {
                            verticalList.predictionsChanged()
                        }
                    }
                }

                MouseArea {
                    height: parent.height
                    width: verticalContainer.inactivePadding
                }
                MouseArea {
                    height: parent.height
                    width: verticalContainer.inactivePadding
                    anchors.right: parent.right
                }
            }
        }
    }

    SilicaPrivate.StringListModel {
        id: suggestionModel

        propertyName: "text"
        strings: MInputMethodQuick.extensions.autoFillSuggestions || []
    }
}
