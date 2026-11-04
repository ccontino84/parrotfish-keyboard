// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The Parrotfish layout: one keyboard for any Latin-script typing language;
// the engine picks the language, this file only gives the keys. Its letter
// keys come from parrotfish/arrangements.js, picked by dconf
// /apps/parrotfish-keyboard/layout (qwerty, qwertz, azerty, nordic). The rows
// are rebuilt when it changes. The keys can have subtle backgrounds
// (parrotfish/, dconf /apps/parrotfish-keyboard/key-fill, key-border). The
// rows can be shorter than Jolla's (/apps/parrotfish-keyboard/key-height in
// portrait, key-height-landscape: 80-100 percent).
// The letter keys' long-press menus show the typing languages' accented
// letters, the others behind › (parrotfish/LongPressMenu.qml).
import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import ".."
import "parrotfish"
import "parrotfish/arrangements.js" as Arrangements

KeyboardLayout {
    id: parrotfishLayout

    // A row of digits above the letters (/apps/parrotfish-keyboard/number-row),
    // /apps/parrotfish-keyboard/number-row-height percent as tall as the
    // other rows (the settings write 100 or 75; default 75: "Smaller", with
    // smaller digits).
    readonly property bool numberRowOn: numberRowConfig.value === true || numberRowConfig.value === "true"
    readonly property int numberRowPercent: {
        var p = parseInt(numberRowHeightConfig.value)
        return p >= 40 && p <= 100 ? p : 75
    }
    // The sticky key (/apps/parrotfish-keyboard/sticky: true, the default;
    // false: Jolla's rows, arrangements.js).
    readonly property bool stickyOn: stickyOnConfig.value !== false && stickyOnConfig.value !== "false"
    readonly property var arrangement: Arrangements.get(arrangementConfig.value, numberRowOn,
                                                        stickySideConfig.value, stickyOn)
    // Shown on the spacebar until the engine knows the language.
    readonly property string arrangementName: arrangement.name

    // Key backgrounds (parrotfish/KeyBackground.qml): fill and border are
    // set separately; the number row's keys look like the others.
    property string keyFill: keyFillConfig.value
    property string keyBorder: keyBorderConfig.value
    // The mark on keys with a long-press menu (parrotfish/LongPressHint.qml):
    // "dot" ("line", an older choice, shows the dot too) or "off".
    readonly property bool longPressHint: longPressHintConfig.value !== "off"
    // The comma's and the period's long-press menus, by typing language.
    readonly property var punctuation: Arrangements.punctuation(languagesConfig.value, Qt.locale().name)
    // The accented letters the letter keys' menus show first (the others
    // are behind ⋯): the typing languages'. The rows are rebuilt when they
    // change.
    readonly property string accentLetters: Arrangements.letters(
        Arrangements.typingLanguages(languagesConfig.value, Qt.locale().name))

    // The sign of the sticky key: the one last picked from its long press.
    readonly property string stickySign: {
        var v = String(stickyConfig.value)
        return v.length === 1 && Arrangements.stickySigns.indexOf(v) >= 0 ? v : "'"
    }

    // The row height in percent of Jolla's, in the current orientation.
    readonly property int keyHeightPercent: _percent(portraitMode ? keyHeightConfig.value
                                                                  : keyHeightLandscapeConfig.value,
                                                     portraitMode ? 85 : 100)

    // The space around each key background, the same across and down: a
    // share of the row height (/apps/parrotfish-keyboard/key-gap percent,
    // default 5), so that shorter rows keep most of their background.
    // Jolla's is fixed (paddingMedium down, paddingSmall across): at 80%
    // rows it took a third of a row.
    readonly property real _gapShare: {
        var p = parseFloat(keyGapConfig.value)
        return p >= 0 && p <= 20 ? p / 100 : 0.05
    }
    readonly property int keyGap: Math.max(1, Math.round(keyHeight * _gapShare))

    // A sticky key in the bottom letter row (QWERTZ, QWERTY (ÅÖÄ)): Shift and
    // Backspace take Jolla's narrow width, as on its layouts with more keys
    // there (tr.qml, uk.qml), so that the letters keep their width.
    readonly property bool _narrowEnds: {
        var last = arrangement.rows[arrangement.rows.length - 1].keys
        for (var i = 0; i < last.length; ++i)
            if (last[i].sticky)
                return true
        return false
    }

    // The keys this layout created, per letter row, and in the number row.
    property var _createdKeys: [[], [], []]
    property var _numberKeys: []

    splitSupported: true

    onArrangementChanged: _build()
    onAccentLettersChanged: _build()
    Component.onCompleted: _build()
    // Jolla's updateSizes() sets keyHeight and then sizes the rows from it:
    // replacing the value here, as it is set, makes the rows use ours. Its
    // value only counts as set if it changes: so scale first, then size.
    onKeyHeightChanged: _scaleKeyHeight()
    onKeyHeightPercentChanged: {
        _scaleKeyHeight()
        updateSizes()
    }
    onAvoidanceWidthChanged: _placeNumbers()

    ConfigurationValue {
        id: arrangementConfig
        key: "/apps/parrotfish-keyboard/layout"   // qwerty, qwertz, azerty, nordic
        defaultValue: Arrangements.preferred(Qt.locale().name)
    }

    ConfigurationValue {
        id: keyFillConfig
        key: "/apps/parrotfish-keyboard/key-fill"   // none, flat, shaded
        defaultValue: "flat"
    }

    ConfigurationValue {
        id: keyBorderConfig
        key: "/apps/parrotfish-keyboard/key-border"   // none, line, raised
        defaultValue: "line"
    }

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"   // "en it de"; "": the system language
        defaultValue: ""
    }

    ConfigurationValue {
        id: stickyConfig
        key: "/apps/parrotfish-keyboard/sticky-key"   // one of Arrangements.stickySigns
        defaultValue: "'"
    }

    ConfigurationValue {
        id: stickyOnConfig
        key: "/apps/parrotfish-keyboard/sticky"   // true, false
        defaultValue: true
    }

    ConfigurationValue {
        id: stickySideConfig
        key: "/apps/parrotfish-keyboard/sticky-side"   // right, left
        defaultValue: "right"
    }

    ConfigurationValue {
        id: keyGapConfig
        key: "/apps/parrotfish-keyboard/key-gap"   // percent of the row height, 0-10 (default 5)
        defaultValue: ""
    }

    ConfigurationValue {
        id: longPressHintConfig
        key: "/apps/parrotfish-keyboard/long-press-hint"   // dot, off
        defaultValue: "dot"
    }

    ConfigurationValue {
        id: keyHeightConfig
        key: "/apps/parrotfish-keyboard/key-height"   // portrait: 80-100 (default 85)
        defaultValue: ""
    }

    ConfigurationValue {
        id: keyHeightLandscapeConfig
        key: "/apps/parrotfish-keyboard/key-height-landscape"   // 80-100 (default 100)
        defaultValue: ""
    }

    ConfigurationValue {
        id: numberRowConfig
        key: "/apps/parrotfish-keyboard/number-row"   // true, false
        defaultValue: true
    }

    ConfigurationValue {
        id: numberRowHeightConfig
        key: "/apps/parrotfish-keyboard/number-row-height"   // 100, 75 (any 40-100)
        defaultValue: "75"
    }

    // The number row: not one of Jolla's rows (their sizing doesn't fit a
    // shorter row), so its keys are placed by _placeNumbers(), with the
    // split gap of the letter rows in landscape.
    Item {
        id: numberRow
        visible: parrotfishLayout.numberRowOn
        height: visible ? Math.round(parrotfishLayout.keyHeight * parrotfishLayout.numberRowPercent / 100) : 0
        onWidthChanged: parrotfishLayout._placeNumbers()
        onHeightChanged: parrotfishLayout._placeNumbers()
    }

    // The letter rows are filled by _build(), in order, so that the keys'
    // order (and the split index) is the arrangement's.
    KeyboardRow { id: row1 }
    KeyboardRow { id: row2 }
    KeyboardRow {
        id: row3
        ShiftKey {
            implicitWidth: parrotfishLayout._narrowEnds ? shiftKeyWidthNarrow : shiftKeyWidth
        }
    }

    BottomRow {}

    LongPressMenu {
        keyboardItem: typeof keyboard !== "undefined" ? keyboard : null
        layout: parrotfishLayout
        onPicked: if (Arrangements.stickySigns.indexOf(sign) >= 0) stickyConfig.value = sign
    }

    Component {
        id: letterKeyComponent
        LetterKey {}
    }

    Component {
        id: backspaceKeyComponent
        BackspaceKey {}
    }

    // A build can be asked for while one is running (creating keys can
    // change accentLetters while the layout loads): then it runs again
    // when that one is done, so that no keys are left behind.
    property bool _building: false
    property bool _buildAgain: false
    function _build() {
        if (_building) {
            _buildAgain = true
            return
        }
        _building = true
        try {
            do {
                _buildAgain = false
                _buildOnce()
            } while (_buildAgain)
        } finally {
            _building = false
        }
    }

    function _buildOnce() {
        var rows = [row1, row2, row3]
        var created = []
        for (var r = 0; r < rows.length; ++r) {
            var old = _createdKeys[r]
            for (var i = 0; i < old.length; ++i) {
                old[i].parent = null   // out of the row now; destroy() is deferred
                old[i].destroy()
            }
            var spec = arrangement.rows[r]
            var keys = []
            for (i = 0; i < spec.keys.length; ++i) {
                var k = spec.keys[i]
                var menus = Arrangements.accentMenus(k.a || "", k.A || "", accentLetters)
                var key = letterKeyComponent.createObject(rows[r], {
                    "caption": k.c, "captionShifted": k.s,
                    "symView": k.y || "", "symView2": k.y2 || "",
                    "accents": menus.a, "accentsShifted": menus.A,
                    "moreAccents": menus.more, "moreAccentsShifted": menus.moreShifted
                })
                if (k.fit)
                    key.fontSizeMode = Text.HorizontalFit
                if (k.sticky) {
                    key.sticky = true
                    key.caption = Qt.binding(function() { return parrotfishLayout.stickySign })
                    key.captionShifted = Qt.binding(function() { return parrotfishLayout.stickySign })
                    key.accents = Arrangements.stickySigns
                    key.accentsShifted = Arrangements.stickySigns
                }
                keys.push(key)
            }
            if (rows[r] === row3) {
                var backspace = backspaceKeyComponent.createObject(row3)
                backspace.implicitWidth = Qt.binding(function() {
                    return parrotfishLayout._narrowEnds ? shiftKeyWidthNarrow : shiftKeyWidth
                })
                keys.push(backspace)
            }
            rows[r].splitIndex = spec.split
            created.push(keys)
        }
        _createdKeys = created
        for (i = 0; i < _numberKeys.length; ++i) {
            _numberKeys[i].parent = null
            _numberKeys[i].destroy()
        }
        var numbers = []
        var digits = arrangement.numbers || []
        for (i = 0; i < digits.length; ++i) {
            numbers.push(letterKeyComponent.createObject(numberRow, {
                "caption": digits[i].c, "captionShifted": digits[i].s,
                "symView": digits[i].y, "symView2": digits[i].y2, "numberRow": true
            }))
        }
        _numberKeys = numbers
        _placeNumbers()
        updateSizes()
    }

    function _placeNumbers() {
        var n = _numberKeys.length
        if (n === 0)
            return
        var gap = avoidanceWidth > 0 ? avoidanceWidth : 0
        var half = Math.floor(n / 2)
        var w = (numberRow.width - gap) / n
        for (var i = 0; i < n; ++i) {
            var key = _numberKeys[i]
            key.x = i * w + (gap > 0 && i >= half ? gap : 0)
            key.width = w
            key.height = numberRow.height
            key.implicitSeparator = i < n - 1 && !(gap > 0 && i === half - 1)
        }
    }

    // A row height setting: 80-100 percent (older values from 50 kept),
    // else `fallback`.
    function _percent(value, fallback) {
        var p = parseInt(value)
        return p >= 50 && p <= 100 ? p : fallback
    }

    function _scaleKeyHeight() {
        var jolla = portraitMode ? geometry.keyHeightPortrait : geometry.keyHeightLandscape
        var scaled = Math.round(jolla * keyHeightPercent / 100)
        if (keyHeight !== scaled)
            keyHeight = scaled
    }
}
