// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick 2.15
import QtTest 1.2
import com.meego.maliitquick 1.0
import com.jolla.keyboard 1.0
import Nemo.Configuration 1.0
import "handlers"

// Our handler (handler/ParrotfishHandler.qml) with the real engine plugin
// and dictionaries, in a stand-in keyboard: keys are tapped by calling
// handleKeyClick() as Jolla's InputHandler does.
Item {
    id: root
    width: 1080; height: 1000

    component Key: Item {
        property int keyType: KeyType.CharacterKey
        property string caption
        property string text: caption
        property int key: 0
        width: 108; height: 150
    }

    // QWERTY as on the phone (1080 px), with or without the number row.
    component Letters: Item {
        id: letters
        property bool numberRow
        property string languageCode: "PARROTFISH"
        property string arrangementName: "QWERTY"
        property int keyHeightPercent: 100
        property bool numberRowOn: numberRow
        width: 1080; height: 800
        Repeater {
            model: letters.numberRow ? "1234567890".split("") : []
            Key { caption: modelData; x: index * 108; y: 0; height: 112 }
        }
        Repeater {
            model: "qwertyuiop".split("")
            Key { caption: modelData; x: index * 108; y: 112 }
        }
        Repeater {
            model: "asdfghjkl'".split("")
            Key { caption: modelData; x: index * 108; y: 262 }
        }
        Repeater {
            model: "zxcvbnm".split("")
            Key { caption: modelData; x: 162 + index * 108; y: 412 }
        }
        Key { id: space; key: Qt.Key_Space; caption: " "; keyType: KeyType.FunctionKey; x: 270; y: 562; width: 540 }
        Key { key: Qt.Key_Backspace; caption: ""; keyType: KeyType.FunctionKey; x: 918; y: 412; width: 162 }
    }

    Letters { id: withNumbers; numberRow: true }
    Letters { id: withoutNumbers; numberRow: false }

    Item {
        id: keyboard
        property Item layout: withNumbers
        property int shiftState: ShiftState.NoShift
        property Item lastPressedKey
        property Item lastInitialKey
        property real dragThreshold: 50
        property bool layoutChangeAllowed: true
        property string mode: "common"
        property string language: "PARROTFISH"
        property bool portraitMode: true
        property bool expandedPaste: false
        property bool autocaps: false

        ParrotfishHandler { id: handler }
    }

    Repeater {
        id: suggestions
        model: handler.engine
        Item { property string word: model.text }
    }
    function bar() {
        handler.engine.flush()   // the event loop's job on the phone
        var out = []
        for (var i = 0; i < suggestions.count; ++i)
            out.push(suggestions.itemAt(i).word)
        return out.join(" ")
    }

    // slips.log lines starting with `how` (the debug file is on).
    function slips(how) {
        var x = new XMLHttpRequest()
        x.open("GET", "file://HOMEPATH/.local/share/parrotfish-keyboard/slips.log", false)
        x.send()
        return x.responseText.split("\n").filter(function(l) { return l.indexOf(how + " ") === 0 })
    }

    function backspace() {
        var rows = keyboard.layout.children
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].key === Qt.Key_Backspace)
                tap(rows[i])
    }

    function keyFor(c) {
        var rows = keyboard.layout.children
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].caption === c && rows[i].visible !== false)
                return rows[i]
        return null
    }

    function tap(k) {
        handler.pressedKey = k
        keyboard.lastPressedKey = k
        keyboard.lastInitialKey = k
        handler.handleKeyClick()
    }

    // A tap that touched `from` and lifted on `to` (Jolla: lastInitialKey).
    function slide(from, to) {
        handler.pressedKey = keyFor(to)
        keyboard.lastPressedKey = keyFor(to)
        keyboard.lastInitialKey = keyFor(from)
        handler.handleKeyClick()
    }

    // Types `s` key by key, then returns what reached the field.
    function type(s) {
        MInputMethodQuick.committedText = ""
        for (var i = 0; i < s.length; ++i) {
            var k = keyFor(s[i])
            if (k === null) {
                // a digit from the symbol page
                k = Qt.createQmlObject('import QtQuick 2.15; import com.jolla.keyboard 1.0; Item { property int keyType: KeyType.CharacterKey; property string text: "' + s[i] + '"; property int key: 0 }', root)
            }
            tap(k)
        }
        handler.engine.flush()
        return MInputMethodQuick.committedText
    }

    TestCase {
        name: "Handler"
        when: windowShown

        function initTestCase() {
            Dconf.values = { "/apps/parrotfish-keyboard/languages": "en",
                             "/apps/parrotfish-keyboard/autocorrect": "normal" }
        }

        function init() {
            MInputMethodQuick.insertsCommits = false
            MInputMethodQuick.showsPreedit = false
            MInputMethodQuick._baseCursor = -1
            MInputMethodQuick.surroundingText = ""
            MInputMethodQuick.cursorPosition = 0
            MInputMethodQuick.surroundingTextValid = false
            keyboard.layout = withNumbers
            handler.active = false
            handler.active = true
            handler._committedDigit = false
        }

        function context(text) {
            MInputMethodQuick.surroundingTextValid = true
            MInputMethodQuick.surroundingText = text
            MInputMethodQuick.cursorPosition = text.length
            handler.active = false
            handler.active = true
        }

        function test_numberRowSlips() {
            compare(handler._digitKeys, true, "digit keys found")
            context("I think this is the ")
            type("tes5")
            compare(handler.preedit, "tes5", "the digit stays in the word")
            verify(["test", "rest"].indexOf(bar().split(" ")[0]) >= 0, "test or rest first: " + bar())
            compare(handler.engine.autocorrection, "", "test or rest: not sure enough (as tesy)")
            compare(type(" "), "tes5 ")
            context("This is an ")
            compare(type("exampl3 "), "example ")
            context("I don't know ")
            compare(type("ab9ut "), "about ")
        }

        function test_tokensStay() {
            compare(type("mp3 "), "MP3 ", "the dictionary has MP3: a capital fix")
            compare(type("ps4 "), "ps4 ")
            compare(type("h2o "), "h2o ")
            compare(type("win7 "), "win7 ")
            compare(type("mp34 "), "mp34 ", "two digits: typed as they are")
            compare(type("h33llo "), "h33llo ")
            compare(type("a4 "), "a4 ", "a letter and a digit: a code")
        }

        function test_afterDigit() {
            compare(type("2nd "), "2nd ", "the digit was just committed")
            compare(type("5om "), "5om ")
            MInputMethodQuick.surroundingTextValid = true
            MInputMethodQuick.surroundingText = "4"
            MInputMethodQuick.cursorPosition = 1
            handler._committedDigit = false
            compare(type("yu "), "yu ", "a digit already in the text")
            MInputMethodQuick.surroundingText = "4 "
            MInputMethodQuick.cursorPosition = 2
            compare(type("tge "), "the ", "after a space: autocorrected")
        }

        // Picks a suggestion, then types `sign`: what reached the field after
        // the pick, and whether the pick's space was deleted.
        function pickThen(word, sign) {
            handler.applyPrediction(word, 0)
            MInputMethodQuick.sentKeys = []
            var text = type(sign)
            return [text, MInputMethodQuick.sentKeys.indexOf(Qt.Key_Backspace) >= 0]
        }

        // A pick ends a word as Space does: the same text, Shift back to
        // automatic, the word counted (slips.log's words line), also for a
        // next word picked with nothing typed.
        function test_pickAsSpace() {
            context("I think it is ")
            keyboard.shiftState = ShiftState.NoShift
            MInputMethodQuick.committedText = ""
            type("goo")
            handler.select("good", 0)
            compare(MInputMethodQuick.committedText, "good ")
            compare(keyboard.shiftState, ShiftState.AutoShift, "typed word picked")
            keyboard.shiftState = ShiftState.NoShift
            var before = slips("words").length
            for (var i = 0; i < 60; ++i) {
                handler.select(bar().split(" ")[0], 0)
                compare(keyboard.shiftState, ShiftState.AutoShift, "next word picked")
                keyboard.shiftState = ShiftState.NoShift
            }
            verify(slips("words").length > before, "next words picked are counted")
        }

        function test_spaceBeforePunctuation() {
            var key = "/apps/parrotfish-keyboard/space-before-punctuation"
            context("I think it is ")
            compare(pickThen("good", "?"), ["? ", true], "default: the space goes")
            compare(pickThen("good", ":"), [": ", true])
            var values = { "/apps/parrotfish-keyboard/languages": "en",
                           "/apps/parrotfish-keyboard/autocorrect": "normal" }
            values[key] = true
            Dconf.values = values
            compare(pickThen("good", "?"), ["? ", false], "on: kept, whatever the language")
            compare(pickThen("good", "!"), ["! ", false])
            compare(pickThen("good", ":"), [": ", false])
            compare(pickThen("good", ";"), ["; ", false])
            compare(pickThen("good", ","), [", ", true], "never before a comma")
            compare(pickThen("good", "."), [". ", true])
            values[key] = "true"   // as dconf may hand it back
            Dconf.values = values
            compare(pickThen("bien", "?"), ["? ", false])
            Dconf.values = { "/apps/parrotfish-keyboard/languages": "en",
                             "/apps/parrotfish-keyboard/autocorrect": "normal" }
        }

        function learningMode(mode) {
            Dconf.values = { "/apps/parrotfish-keyboard/languages": "en",
                             "/apps/parrotfish-keyboard/autocorrect": "normal",
                             "/apps/parrotfish-keyboard/learning": mode }
        }

        // The words a model shows (a PredictionEngine of its own).
        function words(engine) {
            var out = []
            for (var i = 0; i < engine.rowCount(); ++i)
                out.push(engine.data(engine.index(i, 0), 257))   // TextRole
            return out
        }

        // Learning: a new word kept in three
        // texts is learned, suggested, and still there after a restart;
        // Privacy doesn't use it; Off ignores the undo memory too.
        function test_learning() {
            learningMode("on")
            for (var k = 0; k < 3; ++k) {
                context("Yesterday I met ")   // a new text each time (the keyboard reopens)
                compare(type("zorbel "), "zorbel ")
            }
            context("Today I met ")
            type("zorb")
            verify(bar().split(" ").indexOf("zorbel") >= 0, "learned, suggested: " + bar())
            backspace(); backspace(); backspace(); backspace()
            handler.active = false   // saved

            // A new engine, as after a restart.
            var fresh = Qt.createQmlObject('import Parrotfish.Keyboard 1.0; PredictionEngine { languages: "en" }', root)
            var keys = [], items = []
            handler._scan(keyboard.layout, keyboard.layout, keys, items)
            fresh.setKeys(keys)
            fresh.setContext("Today I met ")
            fresh.preedit = "zorb"
            fresh.flush()
            verify(words(fresh).indexOf("zorbel") >= 0, "kept between sessions: " + words(fresh))
            fresh.learning = "privacy"
            fresh.flush()
            verify(words(fresh).indexOf("zorbel") < 0, "privacy: not used: " + words(fresh))
            fresh.destroy()

            learningMode("privacy")
            context("Today I met ")
            type("zorb")
            verify(bar().split(" ").indexOf("zorbel") < 0, "privacy: " + bar())
            backspace(); backspace(); backspace(); backspace()

            // Off: the undo memory isn't used either.
            learningMode("on")
            context("I think ")
            handler.engine.rejectCorrection("tgat")
            handler.engine.rejectCorrection("tgat")
            type("tgat")
            compare(handler.engine.autocorrection, "", "on: two undos stop it")
            learningMode("off")
            handler.engine.flush()
            compare(handler.engine.autocorrection, "that", "off: corrected again")
            backspace(); backspace(); backspace(); backspace()
            learningMode("on")
        }

        // A field that asks not to be learned from (the settings' preview,
        // __inputMethodExtensions): nothing typed there is learned, what was
        // learned is still suggested.
        function test_learnPaused() {
            learningMode("on")
            MInputMethodQuick.extensions = { "parrotfishNoLearning": true }
            verify(handler.engine.learnPaused, "paused")
            for (var k = 0; k < 3; ++k) {
                context("Yesterday I saw ")
                compare(type("quabble "), "quabble ")
            }
            handler.active = false
            handler.active = true
            MInputMethodQuick.extensions = {}
            verify(!handler.engine.learnPaused, "resumed")
            context("Today I saw ")
            type("quab")
            verify(bar().split(" ").indexOf("quabble") < 0, "not learned: " + bar())
            backspace(); backspace(); backspace(); backspace()
        }

        // The settings page (another process: here another object) lists the
        // learned words and forgets one; the keyboard reloads when told.
        function test_learningSettings() {
            learningMode("on")
            for (var k = 0; k < 3; ++k) {
                context("Yesterday I met ")
                compare(type("quorbix "), "quorbix ")
            }
            handler.active = false   // saved (by the keyboard's writer thread)
            var listed = Qt.createQmlObject('import Parrotfish.Keyboard 1.0; LearnedWords {}', root)
            tryVerify(function() { listed.reload(); return words(listed).indexOf("quorbix") >= 0 }, 3000,
                      "listed: " + words(listed))
            // listed under its language; another language's clearing leaves it
            var row = words(listed).indexOf("quorbix")
            compare(listed.data(listed.index(row, 0), 258), "en")          // language
            compare(listed.data(listed.index(row, 0), 259), "English")     // languageName
            compare(listed.languages().map(function(l) { return l.code }).join(" "), "en")
            listed.clearLanguage("it")
            verify(words(listed).indexOf("quorbix") >= 0, "kept: " + words(listed))
            listed.forget("quorbix", "it")
            verify(words(listed).indexOf("quorbix") >= 0, "kept in en: " + words(listed))
            listed.forget("quorbix", "en")
            verify(words(listed).indexOf("quorbix") < 0)
            Dconf.values = { "/apps/parrotfish-keyboard/languages": "en",
                             "/apps/parrotfish-keyboard/autocorrect": "normal",
                             "/apps/parrotfish-keyboard/learning": "on",
                             "/apps/parrotfish-keyboard/learning-changed": Date.now() }
            context("Today I met ")
            type("quorb")
            verify(bar().split(" ").indexOf("quorbix") < 0, "forgotten: " + bar())
            backspace(); backspace(); backspace(); backspace(); backspace()
            verify(listed.languages().length > 0, "en still has counts")
            listed.clearLanguage("en")
            compare(listed.languages().length, 0)
            listed.clearAll()
            compare(listed.count, 0)
            listed.reload()
            compare(listed.count, 0)
            listed.destroy()
        }

        // slips.log counts the suggestions used, and of them the learned ones,
        // on its words line; the header names the learning mode.
        function test_learningSlips() {
            learningMode("on")
            context("I think ")
            compare(type("tgis "), "this ")   // fixed
            type("wor")
            handler.select(bar().split(" ")[0], 0)   // picked
            learningMode("privacy")   // new settings: the words line is written
            context("I think ")
            compare(type("tgat "), "that ")
            var lines = slips("words")
            verify(lines.length > 0, "a words line")
            var last = lines[lines.length - 1]
            verify(/^words \d+ picks \d+\/\d+ next \d+\/\d+ fixes \d+\/\d+$/.test(last), last)
            verify(/picks [1-9]/.test(last) && /fixes [1-9]/.test(last), last)
            var headers = slips("#")
            verify(/ learning=privacy$/.test(headers[headers.length - 1]), headers[headers.length - 1])
            learningMode("on")
        }

        // An autocorrection undone right after Space: the word kept is the
        // one typed (learned as typed, not as the correction).
        function test_learningUndo() {
            learningMode("on")
            for (var k = 0; k < 3; ++k) {
                context("See you ")
                if (k === 2) {   // two undos: the undo memory leaves it
                    compare(type("tomorow "), "tomorow ")
                    continue
                }
                compare(type("tomorow "), "tomorrow ")
                field("See you tomorrow ")
                backspace()   // undo: tomorow again
                compare(MInputMethodQuick.committedText.slice(-8), "tomorow ")
            }
            context("See you ")
            type("tomor")
            verify(bar().split(" ").indexOf("tomorow") >= 0, "learned as typed: " + bar())
            backspace(); backspace(); backspace(); backspace(); backspace()
        }

        // The field's text before the cursor, as the app reports it.
        function field(text) {
            MInputMethodQuick.surroundingTextValid = true
            MInputMethodQuick.surroundingText = text
            MInputMethodQuick.cursorPosition = text.length
        }

        function test_manualCorrection() {
            context("I think ")
            compare(type("tesy "), "tesy ", "not sure enough: typed as it is")
            field("I think tesy ")
            backspace()
            compare(handler.preedit, "tesy", "Backspace goes back into the word")
            backspace()
            compare(type("t "), "test ")
            compare(slips("manual"), ["manual key y t"])
            // Unchanged, or a quite different word: nothing logged.
            field("I think hello ")
            backspace()
            type(" ")
            field("I think hello ")
            backspace()
            for (var i = 0; i < 5; ++i)
                backspace()
            type("table ")
            compare(slips("manual").length, 1)
        }

        function test_lateUndo() {
            context("I think ")
            compare(type("tge "), "the ")
            compare(type("cat "), "cat ")
            field("I think the cat ")
            for (var i = 0; i < 4; ++i)
                backspace()   // "cat " gone, and the space after "the"
            field("I think the ")
            backspace()
            compare(handler.preedit, "the")
            for (i = 0; i < 3; ++i)
                backspace()
            compare(type("tge "), "tge ", "typed as before: kept")
            compare(slips("undone late"), ["undone late key g h"])
        }

        function test_undosKept() {
            var engine = handler.engine
            context("I think ")
            type("tgat")
            engine.flush()
            compare(engine.autocorrection, "that")
            type(" ")
            engine.rejectCorrection("tgat")
            engine.rejectCorrection("tgat")
            // A new engine, as after a restart: the undos are still there.
            var fresh = Qt.createQmlObject('import Parrotfish.Keyboard 1.0; PredictionEngine { languages: "en" }', root)
            var keys = [], items = []
            handler._scan(keyboard.layout, keyboard.layout, keys, items)
            fresh.setKeys(keys)
            fresh.setContext("I think ")
            fresh.preedit = "tgat"
            fresh.flush()
            compare(fresh.autocorrection, "", "not corrected after two undos")
            fresh.preedit = "tgis"
            fresh.flush()
            compare(fresh.autocorrection, "this")
            fresh.destroy()
        }

        function test_undosPerLanguage() {
            var keys = [], items = []
            handler._scan(keyboard.layout, keyboard.layout, keys, items)
            var e = Qt.createQmlObject('import Parrotfish.Keyboard 1.0; PredictionEngine { languages: "en it" }', root)
            e.setKeys(keys)
            var italian = "Non so se domani posso venire, ", english = "I don't know if I can come tomorrow, "
            e.setContext(italian)
            e.preedit = "cpme"
            e.flush()
            compare(e.autocorrection, "come", "corrected in Italian text")
            e.preedit = ""
            e.rejectCorrection("cpme")
            e.rejectCorrection("cpme")
            e.preedit = "cpme"
            e.flush()
            compare(e.autocorrection, "", "not in Italian after two undos")
            e.preedit = ""
            e.setContext(english)
            // Picked from the bar in English (not autocorrected there): the
            // Italian undos stay.
            e.strength = "off"
            e.preedit = "cpme"
            e.flush()
            compare(e.autocorrection, "")
            e.acceptCorrection("cpme")
            e.preedit = ""
            e.strength = "normal"
            e.preedit = "cpme"
            e.flush()
            compare(e.autocorrection, "come", "still corrected in English text")
            e.preedit = ""
            e.setContext(italian)
            e.preedit = "cpme"
            e.flush()
            compare(e.autocorrection, "", "still not in Italian")
            e.destroy()
        }

        // Backspace into a word whose autocorrection was undone, ended as
        // the correction again: a redo, however it is ended.
        function test_redo() {
            var e = handler.engine
            var ways = ["space", "pick", "retype"]
            for (var w = 0; w < ways.length; ++w) {
                // tgat: no undos (another test leaves two)
                e.setContext("I think ")
                e.preedit = "tgat"
                e.rejectCorrection("tgat")
                e.acceptCorrection("tgat")
                e.preedit = ""
                var count = function(how) { return slips(how).length }
                var before = [count("undone"), count("redone"), count("kept"), count("picked"), count("manual")]
                context("I think ")
                compare(type("tgat "), "that ", ways[w])
                field("I think that ")
                backspace()   // undo
                field("I think tgat ")
                backspace()   // back into tgat
                compare(handler.preedit, "tgat")
                if (ways[w] === "space") {
                    compare(type(" "), "that ")
                } else if (ways[w] === "pick") {
                    handler.applyPrediction("that", 0)
                } else {
                    for (var i = 0; i < 3; ++i)
                        backspace()
                    type("hat ")
                }
                type("x")
                compare([count("undone") - before[0], count("redone") - before[1], count("kept") - before[2],
                         count("picked") - before[3], count("manual") - before[4]], [1, 1, 0, 0, 0],
                        ways[w] + ": undone, redone, kept, picked, manual")
                compare(slips("redone").slice(-1), ["redone key g h"])
                // The undo is forgotten: one more doesn't stop the fix.
                e.preedit = ""
                e.setContext("I think ")
                e.preedit = "tgat"
                e.rejectCorrection("tgat")
                e.preedit = ""
                e.preedit = "tgat"
                e.flush()
                compare(e.autocorrection, "that", ways[w] + ": undos forgotten")
                e.preedit = ""
            }
        }

        function test_singleLetter() {
            context("Yesterday after work ")
            compare(type("i "), "I ", "English i at Space")
            context("Yesterday after work ")
            compare(type("i."), "i.", "i. as in i.e.")
            context("Yesterday after work ")
            compare(type("i)"), "i)")
        }

        // Types `before`, closes the keyboard (the word is committed as it
        // is) and opens it again, with the field showing `text`.
        function interrupt(before, text) {
            context(text.substr(0, text.length - before.length))
            type(before)
            handler.active = false
            field(text)
            handler.active = true
        }

        function test_typeOnAfterReturning() {
            // The keyboard closed in the middle of "hel": typing goes on with
            // the same word.
            interrupt("hel", "I think hel")
            MInputMethodQuick.preedits = []
            compare(type("lo "), "hello ")
            compare(MInputMethodQuick.preedits[0], ["hell", -3, 3, undefined], "hel taken back")
            // One word for autocorrect too: tg + e is tge, fixed to the.
            interrupt("tg", "I think tg")
            compare(type("e "), "the ")
            // Only once: then a word at the end of the text is left alone
            // (an Android app reports no text after the cursor: it may be
            // inside a word).
            field("I think hel")
            MInputMethodQuick.preedits = []
            type("p")
            compare(handler.preedit, "p", "not known to be the end of a word")
            handler.reset()
            // Not if the text no longer ends with the interrupted word.
            interrupt("hel", "I think help")
            type("s")
            compare(handler.preedit, "s")
            handler.reset()
            // Where the app reports the text after the cursor (native apps),
            // the end of any word is known.
            field("I think hel world")
            MInputMethodQuick.cursorPosition = 11
            type("p")
            compare(handler.preedit, "help", "end of a word, text after the cursor")
            handler.reset()
            MInputMethodQuick.cursorPosition = 10   // inside "hel"
            type("p")
            compare(handler.preedit, "p", "inside a word: typing inserts")
            handler.reset()
            // After a space: a new word.
            field("I think ")
            MInputMethodQuick.preedits = []
            compare(type("so "), "so ")
            compare(MInputMethodQuick.preedits[0], ["s", undefined, undefined, undefined])
        }

        function test_uncertainTap() {
            context("I think that ")
            MInputMethodQuick.committedText = ""
            type("le")
            slide("f", "d")
            compare(handler.preedit, "led", "led... fits better so far")
            compare(handler.engine.preeditKeys, ["", "", "f1"], "the other key kept")
            type("t")
            compare(handler.engine.preeditKeys, ["", "", "f1", ""])
            compare(handler.engine.autocorrection, "left", "the whole word decides")
            backspace()
            compare(handler.engine.preeditKeys, ["", "", "f1"], "in step after Backspace")
            MInputMethodQuick.committedText = ""
            type("t ")
            compare(MInputMethodQuick.committedText, "left ")
            compare(handler.engine.preeditKeys, [])
        }

        function test_missingSpace() {
            context("I think ")
            compare(type("thisis "), "this is ", "two words typed as one")
            field("I think this is ")
            MInputMethodQuick.committedText = ""
            backspace()
            compare(MInputMethodQuick.committedText, "thisis ", "Backspace brings back what was typed")
            context("I think ")
            type("thisis ")
            type("a")
            handler.reset()
            compare(slips("kept").slice(-1), ["kept space missing"])
        }

        function test_withoutNumberRow() {
            keyboard.layout = withoutNumbers
            handler._updateLayout()
            compare(handler._digitKeys, false)
            compare(type("tes5 "), "tes5 ", "digits from the symbol page end the word")
            compare(type("tge "), "the ")
        }

        // The same text is read once, but the bar still shows the
        // next words whenever no word is being typed.
        function test_nextWordsKept() {
            context("I think this is the ")
            var next = bar()
            verify(next !== "", "next words after the")
            type("xq")
            verify(bar() !== next, "suggestions while typing")
            backspace()
            backspace()
            compare(handler.preedit, "")
            compare(bar(), next, "the next words again after deleting the word")
            context("I think this is the ")
            compare(bar(), next, "the same text again")
            context("I think this is a ")
            verify(bar() !== next, "another text, other next words")
        }

        // After an autocorrection the next words follow the corrected word
        // (until the app reports its text, the handler assumes it).
        function test_nextWordsAfterCorrection() {
            context("I think this is the ")
            var next = bar()
            context("I think this is ")
            compare(type("tge "), "the ")
            compare(bar(), next, "the next words after the, not after tge")
        }

        // After a commit the text before the cursor is the text the app will
        // report, however the field behaves: maliit's
        // copy updated at once (native apps) or not, the word being typed
        // shown in the field (the Android app tested) or not. A period
        // right after a word comes before any report.
        function test_contextAfterCommit_data() {
            return [
                { tag: "native", inserts: true, preedit: false },
                { tag: "android", inserts: false, preedit: true },
                { tag: "both", inserts: true, preedit: true },
                { tag: "neither", inserts: false, preedit: false }
            ]
        }

        function test_contextAfterCommit(data) {
            MInputMethodQuick.insertsCommits = data.inserts
            MInputMethodQuick.showsPreedit = data.preedit
            context("I think this is ")
            // The app reports its text (as after a commit; the stub only
            // changes the field when it inserts commits).
            function report(text) {
                MInputMethodQuick.surroundingText = text
                MInputMethodQuick.cursorPosition = text.length
            }
            compare(type("tge "), "the ")
            compare(handler._loggedContext, "I think this is the ")
            report("I think this is the ")
            compare(handler._loggedContext, "I think this is the ", "the report is what was assumed")
            compare(type("hpuse. "), "house. ")   // house, then . and space before any report
            compare(handler._loggedContext, "I think this is the house. ")
            report("I think this is the house. ")
            compare(handler._loggedContext, "I think this is the house. ")
            compare(type("Ut "), "It ")
            compare(handler._loggedContext, "I think this is the house. It ")
            MInputMethodQuick.showsPreedit = false
            MInputMethodQuick.insertsCommits = false
        }
    }
}
