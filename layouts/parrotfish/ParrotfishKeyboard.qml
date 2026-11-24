// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A Parrotfish layout, for any Latin-script typing language (the engine
// picks the language). Each of Jolla's entries (layouts_parrotfish.conf) is
// this with its `arrangementKey` (arrangements.js).
import QtQuick 2.6
import Sailfish.Silica 1.0
import com.meego.maliitquick 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0
import "../.."
import "arrangements.js" as Arrangements
import "emoji"
import "voice"

KeyboardLayout {
    id: parrotfishLayout

    // The numpad on the symbol view's first page, instead of the number row.
    readonly property bool numpadOn: numpadConfig.value === true || numpadConfig.value === "true"
    // Not in split landscape, which keeps Jolla's page.
    readonly property bool numpadShown: numpadOn && _rowsShown && !splitActive
                                        && attributes.inSymView && !attributes.inSymView2
    // Later: the bottom row's 0 and second spacebar follow it by binding.
    onNumpadShownChanged: _relayoutLater.restart()
    // The number row, number-row-height percent as tall as the other rows.
    readonly property bool numberRowOn: !numpadOn && (numberRowConfig.value === true || numberRowConfig.value === "true")
    readonly property int numberRowPercent: {
        var p = parseInt(numberRowHeightConfig.value)
        return p >= 40 && p <= 100 ? p : 75
    }
    // Off: Jolla's rows (arrangements.js).
    readonly property bool stickyOn: stickyOnConfig.value !== false && stickyOnConfig.value !== "false"
    // The keys: set by each entry's file.
    property string arrangementKey: "qwerty"
    // Its script (Latn, Cyrl, Grek): the typing languages of this layout.
    readonly property string script: Arrangements.scriptOf(arrangementKey)
    // The typing language whose own rows are shown (uk.qml's on ЙЦУКЕН), or "".
    readonly property string _variant: script !== "Latn" ? Arrangements.variantOf(arrangementKey, groupLanguage) : ""
    // The language the switch key shows the keys of (Russian and Ukrainian
    // both typing languages), or "". The keys change only by that key.
    property string _switchTo: ""
    // Other scripts: the language of the keys shown, a hint for the engine.
    readonly property string keysLanguage: script !== "Latn" ? groupLanguage : ""
    readonly property var arrangement: Arrangements.get(arrangementKey, numberRowOn,
                                                        stickySideConfig.value, stickyOn,
                                                        languageKeysOn, languageKeysSideConfig.value,
                                                        _anyLastRowLetter, _variant, _switchTo)

    // The language keys' language (LANG.language's extra-keys): the one the
    // handler remembers (last-language), so that they change between words.
    property string groupLanguage: ""
    readonly property string groupLetters: groupLanguage !== "" ? Languages.extraKeys(groupLanguage) : ""
    property bool _anyGroupLanguage: false
    // A typing language has a letter for the sticky key's place (the 5th of
    // its extra-keys).
    property bool _anyLastRowLetter: false
    readonly property bool languageKeysOn: String(languageKeysConfig.value) === "true"
                                           && _anyGroupLanguage && script === "Latn"
    // Bumped when letters change in place: the handler's check compares places only.
    property int keysRevision: 0
    onGroupLettersChanged: keysRevision++
    // Shown on the spacebar until the engine knows the language.
    readonly property string arrangementName: arrangement.name

    // Key backgrounds (parrotfish/KeyBackground.qml).
    property string keyFill: keyFillConfig.value
    property string keyBorder: keyBorderConfig.value
    // The mark on keys with a long-press menu (parrotfish/LongPressHint.qml).
    readonly property bool longPressHint: longPressHintConfig.value !== "off"
    // The comma's and the period's long-press menus, by typing language.
    readonly property var punctuation: Languages.punctuation(Languages.selected(languagesConfig.value, script))
    // The accented letters the long-press menus show first; a change rebuilds the rows.
    readonly property string accentLetters: Languages.menuLetters(Languages.selected(languagesConfig.value, script))

    // The sign of the sticky key: the one last picked from its long press.
    readonly property string stickySign: {
        var v = String(stickyConfig.value)
        return v.length === 1 && Arrangements.stickySigns.indexOf(v) >= 0 ? v : "'"
    }

    // The row height in percent of Jolla's, in the current orientation.
    readonly property int keyHeightPercent: _percent(portraitMode ? keyHeightConfig.value
                                                                  : keyHeightLandscapeConfig.value,
                                                     portraitMode ? 85 : 100)

    // The spacebar's width in percent of Jolla's (BottomRow.qml): 82-118.
    readonly property int spaceWidthPercent: {
        var p = parseInt(spaceWidthConfig.value)
        return p >= 82 && p <= 118 ? p : 100
    }

    // The space around each key background, a share of the row height
    // (Jolla's fixed padding took a third of an 80% row).
    readonly property real _gapShare: {
        var p = parseFloat(keyGapConfig.value)
        return p >= 0 && p <= 20 ? p / 100 : 0.05
    }
    readonly property int keyGap: Math.max(1, Math.round(keyHeight * _gapShare))
    // The highlight tint of language keys and numpad digits (no setting page).
    readonly property real accentTint: {
        var p = parseFloat(accentTintConfig.value)
        return p >= 0 && p <= 100 ? p / 100 : 0.25
    }

    // A sticky or language key in the bottom letter row: Shift and Backspace
    // take Jolla's narrow width (as in its tr.qml, uk.qml).
    readonly property bool _narrowEnds: {
        if (arrangement.narrowEnds)
            return true
        var last = arrangement.rows[arrangement.rows.length - 1].keys
        for (var i = 0; i < last.length; ++i)
            if (last[i].sticky || last[i].slot !== undefined)
                return true
        return false
    }

    // Keys in the bottom letter row: in split landscape, with six on a half,
    // Shift and Backspace are as wide as a letter.
    property int _row3Count: 0
    function _endWidth() {
        return splitActive && _row3Count >= 11 ? (width - avoidanceWidth) / 12
                                               : _narrowEnds ? shiftKeyWidthNarrow : shiftKeyWidth
    }

    // The emoji panel (emoji/EmojiPanel.qml) instead of the rows.
    property bool emojiView: false
    // Voice input's panel (voice/VoicePanel.qml) instead of the rows.
    property bool voiceView: false
    // The voice package and its model are installed, and voice isn't off.
    readonly property bool voiceEnabled: Voice.available && voiceConfig.value !== false && voiceConfig.value !== "false"
    ConfigurationValue {
        id: voiceConfig
        key: "/apps/parrotfish-keyboard/voice"
        defaultValue: true
    }
    // Jolla's keyboard has no switch for automatic capitals: off, its
    // autocaps is set back to false while this layout is in use.
    readonly property bool automaticCapitals: autocapsConfig.value !== false && autocapsConfig.value !== "false"
    readonly property bool layoutInUse: typeof keyboard !== "undefined" && keyboard.layout === parrotfishLayout
    onAutomaticCapitalsChanged: _updateAutocaps()
    onLayoutInUseChanged: _updateAutocaps()
    function _updateAutocaps() {
        if (typeof keyboard === "undefined")
            return
        if (layoutInUse && !automaticCapitals)
            keyboard.autocaps = false
        else if (typeof keyboard.applyAutocaps === "function")
            keyboard.applyAutocaps()
    }
    ConfigurationValue {
        id: autocapsConfig
        key: "/apps/parrotfish-keyboard/automatic-capitals"   // true (the default), false
        defaultValue: true
    }
    // Greek ends a question with ; which Jolla's automatic capitals don't
    // know: checked after Jolla's (KeyboardBase.applyAutocaps).
    Timer {
        id: greekCapsCheck
        interval: 5
        onTriggered: {
            if (!parrotfishLayout.layoutInUse || !parrotfishLayout.automaticCapitals || keyboard.autocaps
                    || !MInputMethodQuick.surroundingTextValid || MInputMethodQuick.hiddenText)
                return
            var text = MInputMethodQuick.surroundingText.substring(0, MInputMethodQuick.cursorPosition)
            if (/[\u0370-\u03ff\u1f00-\u1fff][;\u037e] $/.test(text))
                keyboard.autocaps = true
        }
    }
    Connections {
        target: parrotfishLayout.script === "Grek" ? MInputMethodQuick : null
        onCursorPositionChanged: greekCapsCheck.restart()
    }

    Connections {
        target: typeof keyboard !== "undefined" ? keyboard : null
        onAutocapsChanged: {
            if (keyboard.autocaps && parrotfishLayout.layoutInUse && !parrotfishLayout.automaticCapitals)
                keyboard.autocaps = false
        }
    }
    readonly property bool _rowsShown: !emojiView && !voiceView
    // Read when the panel opens, so that they don't move while it's open.
    property var emojiRecent: []
    onEmojiViewChanged: if (emojiView) emojiRecent = EmojiUtil.recentList(emojiRecentConfig.value)
    function emojiUsed(emoji) {
        emojiRecentConfig.value = EmojiUtil.recentUse(emojiRecentConfig.value, emoji)
    }
    function emojiForget(emoji) {
        emojiRecentConfig.value = EmojiUtil.recentForget(emojiRecentConfig.value, emoji)
        emojiRecent = EmojiUtil.recentList(emojiRecentConfig.value)
    }
    // The variant last picked per emoji ({ emoji: variant }).
    readonly property var emojiPicked: EmojiUtil.pickedMap(emojiPickedConfig.value)
    function emojiPick(emoji, variant) {
        emojiPickedConfig.value = EmojiUtil.pick(emojiPickedConfig.value, emoji, variant)
    }

    // The keys this layout created, per letter row, and in the number row.
    property var _createdKeys: [[], [], []]
    property var _numberKeys: []

    splitSupported: true

    onArrangementChanged: _build()
    onAccentLettersChanged: _build()
    Component.onCompleted: {
        _updateGroupLanguage()
        _build()
        _buildNumpad()
        if (layoutInUse && !automaticCapitals)
            keyboard.autocaps = false
    }

    function _updateGroupLanguage() {
        var codes = Languages.selected(languagesConfig.value, script)
        if (script !== "Latn") {
            // The keys chosen with the switch key, else the last text's
            // language, else the first.
            var switchable = Arrangements.switchable(arrangementKey, codes)
            var chosen = String(keysConfig.value), remembered = String(lastLanguageConfig.value)
            groupLanguage = switchable.indexOf(chosen) >= 0 ? chosen
                          : codes.indexOf(remembered) >= 0 ? remembered
                          : codes.length > 0 ? codes[0] : ""
            var at = switchable.indexOf(groupLanguage)
            _switchTo = switchable.length > 1 && at >= 0 ? switchable[(at + 1) % switchable.length] : ""
            _anyGroupLanguage = false
            _anyLastRowLetter = false
            return
        }
        var claim = [], lastRow = false
        for (var i = 0; i < codes.length; ++i) {
            var letters = Languages.extraKeys(codes[i])
            if (letters !== "")
                claim.push(codes[i])
            if (letters.length > 4 && letters.charAt(4) !== "-")
                lastRow = true
        }
        var last = String(lastLanguageConfig.value)
        if (claim.indexOf(last) >= 0)
            groupLanguage = last
        else if (claim.indexOf(groupLanguage) < 0)
            groupLanguage = claim.length > 0 ? claim[0] : ""
        _anyGroupLanguage = claim.length > 0
        _anyLastRowLetter = lastRow
    }

    // The letter of a language key at `slot` of the language's letters (0-2
    // the group, 3 the number row's last key, 4 the sticky key's place), or
    // `fallback` for "-".
    function _slotLetter(slot, fallback) {
        var c = groupLetters.charAt(slot)
        return c !== "" && c !== "-" ? c : fallback
    }
    function _bindSlot(key, slot, fallback) {
        key.group = Qt.binding(function() { return parrotfishLayout._slotLetter(slot, "") !== "" })
        key.caption = Qt.binding(function() { return parrotfishLayout._slotLetter(slot, fallback) })
        key.captionShifted = Qt.binding(function() {
            var c = parrotfishLayout._slotLetter(slot, fallback)
            var upper = c.toUpperCase()
            return upper.length === 1 ? upper : c   // ß stays ß
        })
        var menus = function() {
            return Arrangements.menusOf(parrotfishLayout.arrangement,
                                        parrotfishLayout._slotLetter(slot, ""), parrotfishLayout.accentLetters)
        }
        key.accents = Qt.binding(function() { return menus().a })
        key.accentsShifted = Qt.binding(function() { return menus().A })
        key.moreAccents = Qt.binding(function() { return menus().more })
        key.moreAccentsShifted = Qt.binding(function() { return menus().moreShifted })
    }
    // Jolla's updateSizes() sets keyHeight, then sizes the rows from it:
    // replaced here as it is set, the rows use ours.
    onKeyHeightChanged: _scaleKeyHeight()
    onKeyHeightPercentChanged: {
        _scaleKeyHeight()
        updateSizes()
    }
    onSpaceWidthPercentChanged: updateSizes()
    // jolla-keyboard.qml's updateInputHandler() can read `handler` before it
    // follows the field's hints, and keep the wrong one: pick again.
    onHandlerChanged: {
        if (typeof keyboard !== "undefined" && keyboard.layout === parrotfishLayout
                && keyboard.allowLayoutChanges && keyboard.inputHandler !== handler
                && typeof keyboard.updateInputHandler === "function")
            keyboard.updateInputHandler()
    }
    onAvoidanceWidthChanged: _placeNumbers()

    ConfigurationValue {
        id: keyFillConfig
        key: "/apps/parrotfish-keyboard/key-fill"   // none, flat, shaded
        defaultValue: "none"
    }

    ConfigurationValue {
        id: keyBorderConfig
        key: "/apps/parrotfish-keyboard/key-border"   // none, line, raised
        defaultValue: "none"
    }

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"   // "en it de"; "": the system language
        defaultValue: ""
        onValueChanged: parrotfishLayout._updateGroupLanguage()
    }

    ConfigurationValue {
        id: lastLanguageConfig
        // Written by the handler; one per script (last-language-cyrl).
        key: "/apps/parrotfish-keyboard/last-language" + (parrotfishLayout.script === "Latn" ? ""
                                                          : "-" + parrotfishLayout.script.toLowerCase())
        defaultValue: ""
        onValueChanged: parrotfishLayout._updateGroupLanguage()
    }

    ConfigurationValue {
        id: keysConfig
        // The switch key's choice (ru, uk); one per script (keys-cyrl).
        key: "/apps/parrotfish-keyboard/keys-" + parrotfishLayout.script.toLowerCase()
        defaultValue: ""
        onValueChanged: parrotfishLayout._updateGroupLanguage()
    }

    // The switch key: the keys of `code` from now on.
    function switchKeys(code) {
        keysConfig.value = code
    }

    ConfigurationValue {
        id: languageKeysConfig
        key: "/apps/parrotfish-keyboard/language-keys"   // false, true
        defaultValue: false
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
        id: languageKeysSideConfig
        key: "/apps/parrotfish-keyboard/language-keys-side"   // right, left
        defaultValue: "right"
    }
    ConfigurationValue {
        id: stickySideConfig
        key: "/apps/parrotfish-keyboard/sticky-side"   // right, left
        defaultValue: "right"
    }

    ConfigurationValue {
        id: accentTintConfig
        key: "/apps/parrotfish-keyboard/accent-tint"   // percent opacity, 0-100 (default 25)
        defaultValue: ""
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
        id: spaceWidthConfig
        key: "/apps/parrotfish-keyboard/space-width"   // 82-118 (default 100)
        defaultValue: ""
    }

    ConfigurationValue {
        id: numberRowConfig
        key: "/apps/parrotfish-keyboard/number-row"   // true, false
        defaultValue: true
    }

    ConfigurationValue {
        id: numpadConfig
        key: "/apps/parrotfish-keyboard/numpad"   // false, true
        defaultValue: false
    }

    ConfigurationValue {
        id: numberRowHeightConfig
        key: "/apps/parrotfish-keyboard/number-row-height"   // 100, 75 (any 40-100)
        defaultValue: "75"
    }

    // Not one of Jolla's rows, whose sizing doesn't fit a shorter row: placed
    // by _placeNumbers().
    readonly property int _numberRowHeight: numberRowOn ? Math.round(keyHeight * numberRowPercent / 100) : 0

    Item {
        id: numberRow
        visible: parrotfishLayout.numberRowOn && parrotfishLayout._rowsShown
        height: parrotfishLayout._numberRowHeight
        onWidthChanged: parrotfishLayout._placeNumbers()
        onHeightChanged: parrotfishLayout._placeNumbers()
    }

    // Filled by _build().
    KeyboardRow { id: row1; visible: parrotfishLayout._rowsShown && !parrotfishLayout.numpadShown }
    KeyboardRow { id: row2; visible: parrotfishLayout._rowsShown && !parrotfishLayout.numpadShown }
    KeyboardRow {
        id: row3
        visible: parrotfishLayout._rowsShown && !parrotfishLayout.numpadShown
        // KeyboardRow's, with Shift's and Backspace's widths set first (a
        // binding could still have the previous width).
        function maximumBasicButtonWidth(shareableWidth) {
            var end = parrotfishLayout._endWidth()
            if (children[0].implicitWidth !== end)
                children[0].implicitWidth = end
            var last = children[children.length - 1]
            if (last !== children[0] && last.implicitWidth !== end)
                last.implicitWidth = end
            if (avoidanceWidth > 0) {
                var split = splitIndex > 0 ? splitIndex : Math.floor(children.length / 2)
                shareableWidth = (shareableWidth - avoidanceWidth) / 2
                return Math.min(_maximumButtonWidthPart(shareableWidth, 0, split),
                                _maximumButtonWidthPart(shareableWidth, split, children.length))
            }
            return _maximumButtonWidthPart(shareableWidth, 0, children.length)
        }
        ShiftKey {
            Component.onCompleted: parrotfishLayout.useKeyGap(this)
        }
    }

    // In place of the three rows while numpadShown; placed by _placeNumpad().
    Item {
        id: numpadRows
        visible: parrotfishLayout.numpadShown
        height: 3 * parrotfishLayout.keyHeight
        onWidthChanged: parrotfishLayout._placeNumpad()
        onHeightChanged: parrotfishLayout._placeNumpad()
    }

    BottomRow { visible: parrotfishLayout._rowsShown }

    EmojiPanel {
        visible: parrotfishLayout.emojiView
        keyHeight: parrotfishLayout.keyHeight
        recent: parrotfishLayout.emojiRecent
        picked: parrotfishLayout.emojiPicked
        longPressHint: parrotfishLayout.longPressHint
        keyboardKey: parrotfishLayout.splitActive
        current: typeof keyboard !== "undefined" && keyboard.layout === parrotfishLayout
        height: parrotfishLayout._numberRowHeight + 4 * parrotfishLayout.keyHeight
        onVariantPicked: parrotfishLayout.emojiPick(emoji, variant)
        onRecentRemoved: parrotfishLayout.emojiForget(emoji)
        onCloseRequested: parrotfishLayout.emojiView = false
    }

    VoicePanel {
        visible: parrotfishLayout.voiceView
        height: parrotfishLayout._numberRowHeight + 4 * parrotfishLayout.keyHeight
    }

    // No height (the layout is a Column): its menu opens upwards, over the rows.
    LayoutMenu {
        z: 10
        layout: parrotfishLayout
        onVoiceRequested: {
            var handler = keyboard.inputHandler
            if (handler && typeof handler.startVoice === "function")
                handler.startVoice()
        }
    }

    Connections {
        target: MInputMethodQuick
        onActiveChanged: {
            if (!MInputMethodQuick.active) {
                parrotfishLayout.emojiView = false
                parrotfishLayout.voiceView = false
            }
        }
    }

    Timer {
        id: _relayoutLater
        interval: 0
        onTriggered: {
            parrotfishLayout.updateSizes()
            parrotfishLayout._placeNumpad()
        }
    }

    ConfigurationValue {
        id: emojiRecentConfig
        key: "/apps/parrotfish-keyboard/emoji-recent"   // EmojiUtil.recentUse()
        defaultValue: ""
    }

    ConfigurationValue {
        id: emojiPickedConfig
        key: "/apps/parrotfish-keyboard/emoji-variants"   // EmojiUtil.pick()
        defaultValue: ""
    }

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
        id: deadKeyComponent
        DeadLetterKey {}
    }

    Component {
        id: switchKeyComponent
        SwitchKey {}
    }

    // The emoji key of split landscape, which has no suggestion bar.
    Component {
        id: emojiKeyComponent
        EmojiButtonKey { layout: parrotfishLayout }
    }

    Component {
        id: backspaceKeyComponent
        BackspaceKey {
            Component.onCompleted: parrotfishLayout.useKeyGap(this)
        }
    }

    Component {
        id: shiftKeyComponent
        ShiftKey {
            Component.onCompleted: parrotfishLayout.useKeyGap(this)
        }
    }

    // A function key's background (1/2, Backspace) with the layout's gap
    // instead of Jolla's paddingMedium, as the bottom row's and the letters'.
    function useKeyGap(key) {
        for (var j = 0; j < key.children.length; ++j) {
            var item = key.children[j]
            if (item.radius !== undefined && item.styled === undefined && item.anchors
                    && item.anchors.margins === Theme.paddingMedium)
                item.anchors.margins = Qt.binding(function() { return parrotfishLayout.keyGap })
        }
    }

    // The numpad's keys per row: {item, part: "shift", "key" or "backspace"}.
    property var _numpadKeys: []

    function _buildNumpad() {
        var spec = Arrangements.numpad
        var rows = []
        for (var r = 0; r < spec.rows.length; ++r) {
            var last = r === spec.rows.length - 1
            var keys = []
            if (last)
                keys.push({"item": shiftKeyComponent.createObject(numpadRows), "part": "shift"})
            for (var i = 0; i < spec.rows[r].length; ++i) {
                var c = spec.rows[r].charAt(i)
                keys.push({"item": letterKeyComponent.createObject(numpadRows, {
                    "caption": c, "captionShifted": c, "accents": "", "accentsShifted": "",
                    "pad": c >= "0" && c <= "9", "leftPadding": 0, "rightPadding": 0
                }), "part": "key"})
            }
            if (last)
                keys.push({"item": backspaceKeyComponent.createObject(numpadRows), "part": "backspace"})
            rows.push(keys)
        }
        _numpadKeys = rows
        _placeNumpad()
    }

    // Keys a tenth of the width; rows 1-2 start half a key in, their end
    // keys padded to the edges (as Jolla's a-l row), so that row 3, between
    // 1/2 and Backspace 1.5 keys wide, lines up with them.
    function _placeNumpad() {
        var width = numpadRows.width
        if (width <= 0 || _numpadKeys.length === 0)
            return
        var w = width / 10
        for (var r = 0; r < _numpadKeys.length; ++r) {
            var keys = _numpadKeys[r]
            var ended = keys[0].part === "shift"
            for (var i = 0; i < keys.length; ++i) {
                var k = keys[i], key = k.item
                var x, kw = w, padLeft = 0, padRight = 0
                if (k.part === "shift") {
                    x = 0
                    kw = 1.5 * w
                } else if (k.part === "backspace") {
                    x = width - 1.5 * w
                    kw = 1.5 * w
                } else if (ended) {
                    x = 1.5 * w + (i - 1) * w
                } else {
                    x = 0.5 * w + i * w
                    if (i === 0) {
                        x = 0
                        padLeft = 0.5 * w
                    } else if (i === keys.length - 1) {
                        padRight = 0.5 * w
                    }
                    kw = w + padLeft + padRight
                }
                key.x = x
                key.y = r * keyHeight
                key.width = kw
                key.height = keyHeight
                if (key.symView !== undefined) {
                    key.leftPadding = padLeft
                    key.rightPadding = padRight
                    key.implicitSeparator = i !== keys.length - 1
                }
            }
        }
    }

    // Creating keys can change accentLetters, asking for a build during one:
    // it runs again afterwards.
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
            // The split (Arrangements.evenSplit). Row 3 counts Shift and
            // Backspace; its emoji key goes to the shorter half, if even to the
            // language keys' side, so every row's fuller half is on one side.
            var last = rows[r] === row3
            var count = spec.keys.length + (last ? 2 : 0)
            var split = Arrangements.evenSplit(count, spec.split)
            var emojiAt = last ? split : -1   // its index in the row (Shift is 0)
            if (last && (split < count - split
                         || (split === count - split && languageKeysOn
                             && languageKeysSideConfig.value === "left")))
                split += 1
            for (i = 0; i < spec.keys.length; ++i) {
                if (i + 1 === emojiAt)
                    keys.push(emojiKeyComponent.createObject(row3))
                var k = spec.keys[i]
                var menus = Arrangements.accentMenus(k.a || "", k.A || "", accentLetters, k.c)
                if (k.switchTo) {
                    keys.push(switchKeyComponent.createObject(rows[r], {"caption": k.c, "target": k.switchTo,
                                                                        "layout": parrotfishLayout}))
                    continue
                }
                var key = (k.dead ? deadKeyComponent : letterKeyComponent).createObject(rows[r], {
                    "caption": k.c, "captionShifted": k.s,
                    "symView": k.y || "", "symView2": k.y2 || "",
                    "accents": menus.a, "accentsShifted": menus.A,
                    "moreAccents": menus.more, "moreAccentsShifted": menus.moreShifted,
                    "deadKeyAccents": k.d || "", "deadKeyAccentsShifted": k.D || ""
                })
                if (k.fit)
                    key.fontSizeMode = Text.HorizontalFit
                if (k.slot !== undefined)
                    _bindSlot(key, k.slot, k.fb || "")
                if (k.sticky) {
                    key.sticky = true
                    key.caption = Qt.binding(function() { return parrotfishLayout.stickySign })
                    key.captionShifted = Qt.binding(function() { return parrotfishLayout.stickySign })
                    key.accents = Arrangements.stickySigns
                    key.accentsShifted = Arrangements.stickySigns
                }
                keys.push(key)
            }
            if (last) {
                if (emojiAt === spec.keys.length + 1)
                    keys.push(emojiKeyComponent.createObject(row3))
                var backspace = backspaceKeyComponent.createObject(row3)
                backspace.implicitWidth = _endWidth()
                keys.push(backspace)
            }
            rows[r].splitIndex = split
            if (last)
                _row3Count = keys.length + 1   // and Shift
            created.push(keys)
        }
        _createdKeys = created
        keysRevision++
        for (i = 0; i < _numberKeys.length; ++i) {
            _numberKeys[i].parent = null
            _numberKeys[i].destroy()
        }
        var numbers = []
        var digits = arrangement.numbers || []
        for (i = 0; i < digits.length; ++i) {
            var digit = letterKeyComponent.createObject(numberRow, {
                "caption": digits[i].c, "captionShifted": digits[i].s,
                "symView": digits[i].y, "symView2": digits[i].y2, "numberRow": true
            })
            if (digits[i].slot !== undefined) {
                digit.fontSizeMode = Text.HorizontalFit
                _bindSlot(digit, digits[i].slot, digits[i].fb || "")
            }
            numbers.push(digit)
        }
        _numberKeys = numbers
        _placeNumbers()
        updateSizes()
        // Shift's and Backspace's widths are bindings that can update after
        // the keys are placed: place them again once they have.
        _relayoutLater.restart()
    }

    function _placeNumbers() {
        var n = _numberKeys.length
        if (n === 0)
            return
        var gap = avoidanceWidth > 0 ? avoidanceWidth : 0
        var half = arrangement.numbersSplit !== undefined ? arrangement.numbersSplit : Math.floor(n / 2)
        // Split: keys as wide as the fuller half's, centred, as Jolla's rows.
        var side = (numberRow.width - gap) / 2
        var w = gap > 0 ? side / Math.max(half, n - half) : numberRow.width / n
        for (var i = 0; i < n; ++i) {
            var key = _numberKeys[i]
            var right = gap > 0 && i >= half
            var count = gap > 0 ? (right ? n - half : half) : n
            var start = gap > 0 ? (right ? side + gap : 0) + (side - count * w) / 2 : 0
            key.x = start + (right ? i - half : i) * w
            key.width = w
            key.height = numberRow.height
            key.implicitSeparator = i < n - 1 && !(gap > 0 && i === half - 1)
        }
    }

    // A row height setting (50-100 percent), else `fallback`.
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
