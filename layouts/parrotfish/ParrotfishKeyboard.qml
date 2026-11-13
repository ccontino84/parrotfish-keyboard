// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A Parrotfish layout: one keyboard for any Latin-script typing language;
// the engine picks the language, this only gives the keys. Each of
// Jolla's entries (layouts_parrotfish.conf: parrotfish.qml,
// parrotfish_qwertz.qml...) is this with its `arrangementKey` (qwerty,
// qwertz, azerty: arrangements.js), so that Jolla's keyboard
// switches between them. The keys can have subtle backgrounds
// (KeyBackground.qml, dconf /apps/parrotfish-keyboard/key-fill, key-border). The
// rows can be shorter than Jolla's (/apps/parrotfish-keyboard/key-height in
// portrait, key-height-landscape: 80-100 percent).
// The letter keys' long-press menus show the typing languages' accented
// letters, the others behind › (parrotfish/LongPressMenu.qml).
import QtQuick 2.6
import Sailfish.Silica 1.0
import com.meego.maliitquick 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0
import "../.."
import "arrangements.js" as Arrangements
import "emoji"

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
    // The keys: set by each entry's file.
    property string arrangementKey: "qwerty"
    readonly property var arrangement: Arrangements.get(arrangementKey, numberRowOn,
                                                        stickySideConfig.value, stickyOn,
                                                        languageKeysOn, languageKeysSideConfig.value,
                                                        _anyLastRowLetter)

    // Language keys (since 1.1; the Look page's "Language
    // keys"): three keys whose letters are those of the typing language
    // (LANG.language's extra-keys), tinted (KeyBackground's groupLook). They follow the language the handler remembers after
    // each committed word it is sure of (last-language), so they change
    // only between words; a language without such letters (English) leaves
    // them as they are. Shown only if a typing language has such letters.
    property string groupLanguage: ""
    readonly property string groupLetters: groupLanguage !== "" ? Languages.extraKeys(groupLanguage) : ""
    property bool _anyGroupLanguage: false
    // A typing language has a letter for the sticky key's place (the 5th of
    // its extra-keys): only then does that place get a language key while
    // the sticky key is off.
    property bool _anyLastRowLetter: false
    // language-keys: false (the default: they narrow the letters on QWERTY
    // and AZERTY), true.
    readonly property bool languageKeysOn: String(languageKeysConfig.value) === "true"
                                           && _anyGroupLanguage
    // Bumped when the keys' letters change without a rebuild, so that the
    // handler reads the keys again (its quick check only compares places).
    property int keysRevision: 0
    onGroupLettersChanged: keysRevision++
    // Shown on the spacebar until the engine knows the language.
    readonly property string arrangementName: arrangement.name

    // Key backgrounds (parrotfish/KeyBackground.qml): fill and border are
    // set separately; the number row's keys look like the others.
    property string keyFill: keyFillConfig.value
    property string keyBorder: keyBorderConfig.value
    // The mark on keys with a long-press menu (parrotfish/LongPressHint.qml):
    // "dot" or "off".
    readonly property bool longPressHint: longPressHintConfig.value !== "off"
    // The comma's and the period's long-press menus, by typing language.
    readonly property var punctuation: Languages.punctuation(Languages.selected(languagesConfig.value))
    // The accented letters the letter keys' menus show first (the others
    // are behind ⋯): the typing languages'. The rows are rebuilt when they
    // change.
    readonly property string accentLetters: Languages.menuLetters(Languages.selected(languagesConfig.value))

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

    // A sticky key or a language key in the bottom letter row (QWERTZ,
    // QWERTY with the language keys): Shift and Backspace take
    // Jolla's narrow width, as on its layouts with more keys there (tr.qml,
    // uk.qml), so that the letters keep their width.
    readonly property bool _narrowEnds: {
        var last = arrangement.rows[arrangement.rows.length - 1].keys
        for (var i = 0; i < last.length; ++i)
            if (last[i].sticky || last[i].slot !== undefined)
                return true
        return false
    }

    // Shift's and Backspace's width. In split landscape, with six keys on a
    // half of the bottom letter row (the emoji key, and a language key or
    // the sticky key there), as wide as a letter key of a six-key half, so
    // that the letters keep their width.
    property int _row3Count: 0
    readonly property real _endWidth: splitActive && _row3Count >= 11
                                      ? (width - avoidanceWidth) / 12
                                      : _narrowEnds ? shiftKeyWidthNarrow : shiftKeyWidth

    // The emoji panel (emoji/EmojiPanel.qml) instead of the rows; off when
    // the keyboard closes or Jolla's handler takes over (passwords, numbers).
    property bool emojiView: false
    // The recent emoji: read when the panel opens, so that they don't move
    // while it is open.
    property var emojiRecent: []
    onEmojiViewChanged: if (emojiView) emojiRecent = EmojiUtil.recentList(emojiRecentConfig.value)
    function emojiUsed(emoji) {
        emojiRecentConfig.value = EmojiUtil.recentUse(emojiRecentConfig.value, emoji)
    }
    // Removed from the recent emoji by hand: gone from the panel at once.
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
    }

    function _updateGroupLanguage() {
        var codes = Languages.selected(languagesConfig.value)
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

    // The letter of a language key: its place in the language's letters
    // (0-2 the group, 3 the number row's last key, 4 the sticky key's place;
    // "-": none, the key types `fallback`). Its long-press menus are those of
    // the key that has the letter among its accents (è: E's).
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
    // Jolla's updateSizes() sets keyHeight and then sizes the rows from it:
    // replacing the value here, as it is set, makes the rows use ours. Its
    // value only counts as set if it changes: so scale first, then size.
    onKeyHeightChanged: _scaleKeyHeight()
    onKeyHeightPercentChanged: {
        _scaleKeyHeight()
        updateSizes()
    }
    onSpaceWidthPercentChanged: updateSizes()
    // Jolla's keyboard picks the handler again when the field's hints change
    // (jolla-keyboard.qml: updateInputHandler()), but can read `handler`
    // before it follows them, and keep the wrong one: pick again.
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
        key: "/apps/parrotfish-keyboard/last-language"   // written by the handler
        defaultValue: ""
        onValueChanged: parrotfishLayout._updateGroupLanguage()
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
        id: numberRowHeightConfig
        key: "/apps/parrotfish-keyboard/number-row-height"   // 100, 75 (any 40-100)
        defaultValue: "75"
    }

    // The number row: not one of Jolla's rows (their sizing doesn't fit a
    // shorter row), so its keys are placed by _placeNumbers(), with the
    // split gap of the letter rows in landscape.
    readonly property int _numberRowHeight: numberRowOn ? Math.round(keyHeight * numberRowPercent / 100) : 0

    Item {
        id: numberRow
        visible: parrotfishLayout.numberRowOn && !parrotfishLayout.emojiView
        height: parrotfishLayout._numberRowHeight
        onWidthChanged: parrotfishLayout._placeNumbers()
        onHeightChanged: parrotfishLayout._placeNumbers()
    }

    // The letter rows are filled by _build(), in order, so that the keys'
    // order (and the split index) is the arrangement's.
    KeyboardRow { id: row1; visible: !parrotfishLayout.emojiView }
    KeyboardRow { id: row2; visible: !parrotfishLayout.emojiView }
    KeyboardRow {
        id: row3
        visible: !parrotfishLayout.emojiView
        ShiftKey {
            implicitWidth: parrotfishLayout._endWidth
        }
    }

    BottomRow { visible: !parrotfishLayout.emojiView }

    // As tall as what it replaces: the number row, three letter rows and the
    // bottom row.
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

    Connections {
        target: MInputMethodQuick
        onActiveChanged: if (!MInputMethodQuick.active) parrotfishLayout.emojiView = false
    }

    Timer {
        id: _relayoutLater
        interval: 0
        onTriggered: parrotfishLayout.updateSizes()
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

    // The emoji key of split landscape (emoji/EmojiButtonKey.qml), where there is
    // no suggestion bar for the handler's emoji button: in the bottom letter
    // row, at the inner edge of its half with fewer keys.
    Component {
        id: emojiKeyComponent
        EmojiButtonKey { layout: parrotfishLayout }
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
            // The split: as many keys on each half (Arrangements.evenSplit).
            // Row 3 counts Shift and Backspace, and gets the emoji key at the
            // inner edge of its shorter half; if even, on the language keys'
            // side, where the other rows have their extra key (right without
            // them), so that every row's fuller half is on the same side.
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
                var menus = Arrangements.accentMenus(k.a || "", k.A || "", accentLetters)
                var key = letterKeyComponent.createObject(rows[r], {
                    "caption": k.c, "captionShifted": k.s,
                    "symView": k.y || "", "symView2": k.y2 || "",
                    "accents": menus.a, "accentsShifted": menus.A,
                    "moreAccents": menus.more, "moreAccentsShifted": menus.moreShifted
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
                backspace.implicitWidth = Qt.binding(function() { return parrotfishLayout._endWidth })
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
        // Shift's and Backspace's widths (_narrowEnds) are bindings that can
        // update after this updateSizes() has placed the keys (seen when the
        // language keys are turned off: keys overlapped until the next
        // resize). Place them again once all bindings have their values.
        _relayoutLater.restart()
    }

    function _placeNumbers() {
        var n = _numberKeys.length
        if (n === 0)
            return
        var gap = avoidanceWidth > 0 ? avoidanceWidth : 0
        var half = arrangement.numbersSplit !== undefined ? arrangement.numbersSplit : Math.floor(n / 2)
        // Split: each half on its side of the gap, its keys as wide as those
        // of the fuller half and centred, as Jolla's rows place theirs.
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
