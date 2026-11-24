// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
import QtQuick 2.15
import QtTest 1.2
import Nemo.Configuration 1.0
import MeeGo.Connman 0.2
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

// Every page loads and shows what dconf holds. run.sh's data: fr, uk and
// el not installed (to download), a downloaded de that is not this
// release's (an update).
Item {
    id: root
    width: 540
    height: 960

    // The pages push others through pageStack; record what they push.
    QtObject {
        id: pageStack
        property var pushed: []
        function animatorPush(url, props) { pushed = pushed.concat([String(url)]) }
        function push(url, props) { animatorPush(url, props) }
        function pop() {}
    }

    TestCase {
        name: "Settings"
        when: windowShown

        function load(file, props) {
            var c = Qt.createComponent(file)
            verify(c.status === Component.Ready, file + ": " + c.errorString())
            var o = c.createObject(root, props || {})
            verify(o !== null, file)
            return o
        }

        // The first item passing `test`, among the shown ones only.
        function find(item, test) {
            if (!item.visible)
                return null
            if (test(item))
                return item
            var kids = item.children || []
            for (var i = 0; i < kids.length; ++i) {
                var f = find(kids[i], test)
                if (f)
                    return f
            }
            return null
        }

        function findAll(item, test, out) {
            out = out || []
            if (!item.visible)
                return out
            if (test(item))
                out.push(item)
            var kids = item.children || []
            for (var i = 0; i < kids.length; ++i)
                findAll(kids[i], test, out)
            return out
        }

        function byLabel(page, label) {
            return find(page, function(i) { return i.label === label || i.text === label || i.title === label })
        }

        // A language's row: a switch row, or a download/update row (one
        // with `languagesPage`), shown.
        function row(page, code, download) {
            return find(page, function(i) {
                return i.language !== undefined && i.language.code === code
                       && (i.languagesPage !== undefined) === !!download
            })
        }

        // The texts of the shown section headers, in order.
        function headers(page) {
            return findAll(page, function(i) { return i.toString().indexOf("SectionHeader") === 0 })
                   .map(function(i) { return i.text })
        }

        function init() {
            Dconf.values = {}
            MobileData.route = "wifi"
            Languages.setSystemLocale("en_GB")
        }

        function test_section() {
            var s = load("ParrotfishSettings.qml")
            verify(byLabel(s, "Version dev") !== null, "version shown")
            compare(byLabel(s, "Appearance").value, "No fill, no border")
            compare(byLabel(s, "Layout").value, "Number row, sticky key")
            Dconf.values = {"/apps/parrotfish-keyboard/key-fill": "shaded",
                            "/apps/parrotfish-keyboard/key-border": "line",
                            "/apps/parrotfish-keyboard/languages": "en it"}
            compare(byLabel(s, "Appearance").value, "Shaded, line")
            compare(byLabel(s, "Typing").value, "English, Italiano, Русский")
            Dconf.values = {"/apps/parrotfish-keyboard/number-row": false,
                            "/apps/parrotfish-keyboard/language-keys": true}
            compare(byLabel(s, "Layout").value, "Language keys, sticky key")
            Dconf.values = {"/apps/parrotfish-keyboard/numpad": true}
            compare(byLabel(s, "Layout").value, "Numpad, sticky key")
            Dconf.values = {"/apps/parrotfish-keyboard/number-row": false,
                            "/apps/parrotfish-keyboard/sticky": false}
            compare(byLabel(s, "Layout").value, "Off")
            verify(byLabel(s, "Voice input") === null, "no voice helper on the PC")
            var pages = [["Typing", "ParrotfishTyping.qml"], ["Layout", "ParrotfishLayout.qml"],
                         ["Appearance", "ParrotfishAppearance.qml"]]
            for (var i = 0; i < pages.length; ++i) {
                byLabel(s, pages[i][0]).clicked(null)
                var last = String(pageStack.pushed[pageStack.pushed.length - 1])
                verify(last.indexOf(pages[i][1]) >= 0, pages[i][0] + " opens " + last)
            }
            s.destroy()
        }

        // The language list (languages.json, run.sh's data).
        function test_languages() {
            compare(Languages.all.length, 20)
            verify(!Languages.all.filter(function(l) { return l.code === "cs" })[0].experimental)
            verify(Languages.all.filter(function(l) { return l.code === "hr" })[0].experimental)
            compare(Languages.script("uk"), "Cyrl")
            compare(Languages.script("el"), "Grek")
            verify(!Languages.isInstalled("fr") && Languages.isInstalled("ru"))
            compare(Languages.names(["it", "en"]), "English, Italiano")
            compare(Languages.toggled(["en", "it"], "de"), "de en it")
            compare(Languages.toggled(["en", "it"], "it"), "en")
            compare(Languages.selected("it bogus en").join(" "), "en it")
            compare(Languages.typing("fr it").join(" "), "it", "fr not installed")
            compare(Languages.typingAll("en").join(" "), "en ru")
            compare(Languages.typingAll("en", ["en.qml", "parrotfish_azerty.qml"]).join(" "), "en", "no Cyrillic layout")
            compare(Languages.typingAll("en", ["en.qml"]).join(" "), "en ru", "no Parrotfish layout: all")
            compare(Languages.scriptsOn(["parrotfish_greek.qml", "parrotfish.qml", "parrotfish_qwertz.qml"]), ["Grek", "Latn"])
            compare(Languages.menuLetters(["de", "it"]), "äöüßàèéìòù")
            compare(Languages.punctuation(["es"]).period, "¡!.?¿")
            compare(Languages.punctuation(["en"]).comma, ":,;—")
            compare(Languages.punctuation(["it"]).comma, ":,;")
            var cases = [["it_IT", "it"], ["pt_AO", "pt_PT"], ["pt_BR", "pt_BR"], ["nn_NO", "nb"], ["de_CH", "de"],
                         ["fi_FI", "fi"], ["ja_JP", "en"]]
            for (var i = 0; i < cases.length; ++i) {
                Languages.setSystemLocale(cases[i][0])
                compare(Languages.selected("").join(" "), cases[i][1], cases[i][0])
            }
        }

        function test_typing() {
            var p = load("ParrotfishTyping.qml")
            compare(byLabel(p, "Typing languages").value, "English, Русский")
            compare(byLabel(p, "Autocorrect").currentIndex, 1)   // Normal, the default
            compare(byLabel(p, "Autocorrect").description, "Fixes words not in the dictionaries")
            var preview = find(p, function(i) { return i.__inputMethodExtensions !== undefined })
            verify(preview !== null, "preview")
            var clear = find(preview.parent, function(i) { return i.icon !== undefined && String(i.icon.source).indexOf("clear") >= 0 })
            verify(clear === null, "no clear button while empty")
            preview.text = "some text"
            clear = find(preview.parent, function(i) { return i.icon !== undefined && String(i.icon.source).indexOf("clear") >= 0 })
            verify(clear !== null, "clear button shown")
            clear.clicked(null)
            compare(preview.text, "")
            Dconf.values = {"/apps/parrotfish-keyboard/learning": "privacy"}
            compare(byLabel(p, "Learning").currentIndex, 1)
            compare(byLabel(p, "Learning").description, "Suggests nothing learned; only stops corrections you undo")
            byLabel(p, "Typing languages").clicked(null)
            verify(pageStack.pushed[pageStack.pushed.length - 1].indexOf("ParrotfishLanguages.qml") >= 0)
            p.destroy()
        }

        function test_typingAids() {
            var p = load("ParrotfishTyping.qml")
            compare(byLabel(p, "Typing aids").value, "2 on", "the default")
            Dconf.values = {"/apps/parrotfish-keyboard/double-space-period": false,
                            "/apps/parrotfish-keyboard/automatic-capitals": false}
            compare(byLabel(p, "Typing aids").value, "Off")
            byLabel(p, "Typing aids").clicked(null)
            verify(pageStack.pushed[pageStack.pushed.length - 1].indexOf("ParrotfishTypingAids.qml") >= 0)
            p.destroy()
            Dconf.values = {}
            var a = load("ParrotfishTypingAids.qml")
            verify(find(a, function(i) { return i.__inputMethodExtensions !== undefined }) === null, "no preview")
            verify(byLabel(a, "Period on double space").checked, "on by default")
            verify(!byLabel(a, "Replace emoticons").checked)
            byLabel(a, "Period on double space").clicked(null)
            verify(!byLabel(a, "Period on double space").checked, "switched off")
            a.destroy()
        }

        function test_layout() {
            var p = load("ParrotfishLayout.qml")
            var digits = byLabel(p, "Digits")
            compare(digits.currentIndex, 2)   // Smaller row
            compare(byLabel(p, "Sticky key").currentIndex, 1)   // on the right
            compare(byLabel(p, "Language keys").currentIndex, 0)   // off by default
            verify(byLabel(p, "Swipe to switch keyboards").checked)
            verify(find(p, function(i) { return i.__inputMethodExtensions !== undefined }) !== null, "preview")
            Dconf.values = {"/apps/parrotfish-keyboard/number-row": false,
                            "/apps/parrotfish-keyboard/sticky": false}
            compare(digits.currentIndex, 0)
            compare(digits.description, "Digits behind the ?123 key")
            compare(byLabel(p, "Sticky key").currentIndex, 0)   // off
            Dconf.values = {"/apps/parrotfish-keyboard/numpad": true}
            compare(digits.currentIndex, 3)
            compare(digits.description, "A keypad behind the ?123 key")
            Dconf.values = {"/apps/parrotfish-keyboard/number-row-height": "100"}
            compare(digits.currentIndex, 1)
            Dconf.values = {"/apps/parrotfish-keyboard/language-keys": true,
                            "/apps/parrotfish-keyboard/language-keys-side": "left"}
            compare(byLabel(p, "Language keys").currentIndex, 2)   // on the left
            var none = "None of your typing languages uses them"
            Dconf.values = {"/apps/parrotfish-keyboard/language-keys": true, "/apps/parrotfish-keyboard/languages": "en"}
            compare(byLabel(p, "Language keys").description, none)   // English has none
            Dconf.values = {"/apps/parrotfish-keyboard/language-keys": true, "/apps/parrotfish-keyboard/languages": "en de"}
            verify(byLabel(p, "Language keys").description !== none, "German has some")
            Dconf.values = {"/sailfish/text_input/active_layout": "parrotfish_greek.qml"}
            compare(byLabel(p, "Sticky key").description, "Only for the Latin layouts")
            verify(!byLabel(p, "Sticky key").enabled)
            Dconf.values = {"/apps/parrotfish-keyboard/swipe": "off"}
            verify(!byLabel(p, "Swipe to switch keyboards").checked)
            p.destroy()
        }

        function test_appearance() {
            var p = load("ParrotfishAppearance.qml")
            compare(byLabel(p, "Key height in portrait").value, 85)
            compare(byLabel(p, "Key height in landscape").value, 100)
            compare(byLabel(p, "Space between keys").value, 5)
            var space = byLabel(p, "Space key width")
            compare(space.value, 100)
            compare([space.minimumValue, space.maximumValue, space.stepSize], [82, 118, 6])
            Dconf.values = {"/apps/parrotfish-keyboard/space-width": "200"}
            compare(space.value, 100)   // out of range: the default
            Dconf.values = {"/apps/parrotfish-keyboard/space-width": "112"}
            compare(space.value, 112)
            compare(byLabel(p, "Key fill").currentIndex, 0)
            compare(byLabel(p, "Key border").currentIndex, 0)
            verify(byLabel(p, "Long-press hint").checked)
            verify(byLabel(p, "Swipe to switch keyboards") === null, "moved to Layout")
            var field = find(p, function(i) { return i.__inputMethodExtensions !== undefined })
            verify(field !== null && field.__inputMethodExtensions.parrotfishNoLearning === true)
            p.destroy()
        }

        function test_languagesPage() {
            var p = load("ParrotfishLanguages.qml")
            // The Keyboards key unread: every alphabet with a language installed.
            compare(headers(p), ["Latin", "Cyrillic", "Updates", "Download"])
            compare(p.typed.join(" "), "en ru")
            verify(row(p, "en") && row(p, "en").on, "en"); verify(row(p, "ru") && row(p, "ru").on, "ru"); verify(row(p, "it") && !row(p, "it").on, "it")
            verify(row(p, "fr", true) !== null && row(p, "uk", true) !== null && row(p, "el", true) !== null)
            verify(row(p, "fr") === null, "no switch for a language not installed")
            // Download: by alphabet.
            var groups = findAll(p, function(i) { return i.script !== undefined && i.count !== undefined })
            compare(groups.map(function(g) { return g.script }), ["Latn", "Cyrl", "Grek"])

            // Switches: one stays on in each alphabet.
            row(p, "it").clicked(null)
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], "en it ru")
            row(p, "en").clicked(null)
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], "it ru")
            row(p, "it").clicked(null)
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], "it ru", "the last Latin one")
            compare(p.keptOn, "it")
            row(p, "ru").clicked(null)
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], "it ru", "the last Cyrillic one")
            compare(p.keptOn, "ru")

            // Keyboards: the Greek layout on, the Cyrillic one off.
            Dconf.values = {"/sailfish/text_input/enabled_layouts": ["en.qml", "parrotfish.qml", "parrotfish_greek.qml"]}
            compare(headers(p), ["Latin", "Cyrillic", "Greek", "Updates", "Download"])
            verify(byLabel(p, "Turn on Auto (ЙЦУКЕН) in Keyboards to type these.") !== null)
            verify(byLabel(p, "No languages yet: download one below.") !== null, "Greek")
            p.keptOn = ""
            verify(!row(p, "ru").on, "Cyrillic off: its languages too")
            row(p, "ru").clicked(null)
            compare(p.keptOn, "", "Cyrillic off: nothing happens")
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], undefined)
            // A choice of an alphabet that is off is kept.
            var v = Object.assign({}, Dconf.values)
            v["/apps/parrotfish-keyboard/languages"] = "en ru"
            Dconf.values = v
            row(p, "it").clicked(null)
            compare(Dconf.values["/apps/parrotfish-keyboard/languages"], "en it ru")

            // Mobile data: the second tap downloads.
            MobileData.route = "cellular"
            verify(p.mobileData)
            row(p, "fr", true).clicked(null)
            compare(p.armed, "fr")
            p.destroy()
        }

        function test_updates() {
            var p = load("ParrotfishLanguages.qml")
            verify(row(p, "de", true) !== null, "de's update")
            compare(row(p, "de").language.source, "download", "tried first, though not this release's")
            compare(p.updateCount, 1)
            p.destroy()
        }

        function test_voice() {
            var p = load("ParrotfishVoice.qml")
            verify(byLabel(p, "Voice input") !== null)
            p.destroy()
        }

        function test_learned() {
            var p = load("ParrotfishLearned.qml", {"learned": Qt.createQmlObject(
                "import Parrotfish.Keyboard 1.0; LearnedWords {}", root)})
            verify(byLabel(p, "Learned words") !== null)
            p.destroy()
        }

        // KeyboardInfo reads the real dconf (run.sh seeds autocorrect,
        // languages, number-row in a private session).
        function test_diagnostics() {
            var p = load("ParrotfishDiagnostics.qml", {"info": Qt.createQmlObject(
                "import Parrotfish.Keyboard 1.0; KeyboardInfo {}", root)})
            verify(p.text.indexOf("Parrotfish dev") === 0, p.text)
            verify(p.text.indexOf("\nautocorrect normal\n") >= 0, p.text)
            verify(p.text.indexOf("\nnumber-row false\n") >= 0, p.text)
            verify(p.text.indexOf("\nenabled_layouts ") >= 0, p.text)
            verify(p.text.indexOf("\nkey-fill -\n") >= 0, p.text)
            verify(p.text.indexOf("\ndictionaries ") >= 0, p.text)
            p.destroy()
        }

        // Reset asks first (remorse), then unsets every key, hidden and
        // old ones too (run.sh seeds sticky-key and key-tuning, and checks
        // that nothing is left afterwards).
        function test_resetAndReport() {
            var s = load("ParrotfishSettings.qml")
            var info = Qt.createQmlObject("import Parrotfish.Keyboard 1.0; KeyboardInfo {}", root)
            byLabel(s, "Reset").clicked()
            verify(Remorse.last !== null, "remorse first")
            compare(Remorse.last.text, "Settings back to defaults")
            verify(info.diagnostics().indexOf("\nautocorrect normal\n") >= 0, "not before the timer")
            Remorse.last.fn()
            var text = info.diagnostics()
            verify(text.indexOf("\nautocorrect -\n") >= 0, text)
            verify(text.indexOf("\nnumber-row -\n") >= 0, text)
            verify(text.indexOf("\nlanguages -\n") >= 0, "languages too: " + text)
            s.destroy()
        }
    }
}
