// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-TBD

// Parrotfish prediction handler for jolla-keyboard.
//
// Based on PreeditHandler.qml of jolla-keyboard 0.10.4 (Jolla Ltd.,
// BSD-3-Clause, see LICENSES/BSD-3-Clause.txt): the word being typed is
// kept as preedit and committed on Space, punctuation or Return. Changes:
// - suggestions and next words come from our engine (Parrotfish.Keyboard);
// - Gentle autocorrect: Space or punctuation commits the engine's
//   correction, shown first and highlighted in the bar (the typed word
//   stays in the text field and is not repeated in the bar);
// - Backspace straight after an autocorrection brings back what was typed;
// - a tap that starts on one letter key and lifts on another types the
//   letter that makes the more likely word;
// - the real key geometry and the text before the cursor go to the engine;
// - the spacebar shows the typing language;
// - swiping to another layout can be switched off
//   (dconf /apps/parrotfish-keyboard/swipe: normal, off);
// - an empty field leans towards the language the last text was in
//   (dconf /apps/parrotfish-keyboard/last-language);
// - thin dividers between the suggestions (dconf
//   /apps/parrotfish-keyboard/dividers: true, false); the suggestion
//   delegates are those of HorizontalPredictionListView.qml and
//   VerticalPredictionListView.qml (Jolla Ltd., BSD-3-Clause) plus a divider.
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
    readonly property bool showDividers: dividersConfig.value !== false && dividersConfig.value !== "false"
    readonly property int dividerWidth: Math.max(1, Math.round(Theme.pixelRatio))
    property int candidateSpaceIndex: -1
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

    // The last autocorrection, while Backspace can still undo it:
    // { typed, corrected, suffix }.
    property var _correction: null
    property string _lastPreedit: ""
    // True while commit() runs: an emptied preedit is then a committed word,
    // not a word deleted with Backspace.
    property bool _committing: false
    // The layout whose keys the engine has, its width (rotation) and its
    // keysRevision (bumped when the Parrotfish layout rebuilds its keys).
    property Item _keysLayout: null
    property real _keysWidth: 0
    property int _keysRevision: -1
    property Item _spacebar: null
    // The spacebar's text item, switched to styled text so that the language
    // can be shown in italics; null if it can't be found.
    property Item _spacebarLabel: null
    // The keyboard's own drag threshold, restored when we're not active.
    property real _jollaDragThreshold: -1

    PredictionEngine {
        id: predictionEngine
        language: inputHandler.language
        strength: strengthConfig.value
        tuning: tuningConfig.value
        onLeadingLanguageChanged: inputHandler._updateSpacebar()
        // New parameters reset the engine: pass the keys again.
        onTuningChanged: inputHandler._keysLayout = null
        onStrengthChanged: inputHandler._keysLayout = null
    }

    ConfigurationValue {
        id: lastLanguageConfig
        key: "/apps/parrotfish-keyboard/last-language"   // en, it, de
        defaultValue: ""
    }

    ConfigurationValue {
        id: swipeConfig
        key: "/apps/parrotfish-keyboard/swipe"   // normal, off
        defaultValue: "normal"
        onValueChanged: inputHandler._applySwipe()
    }

    ConfigurationValue {
        id: dividersConfig
        key: "/apps/parrotfish-keyboard/dividers"   // true, false
        defaultValue: true
    }

    ConfigurationValue {
        id: tuningConfig
        key: "/apps/parrotfish-keyboard/tuning"   // hidden: "name=value name=value"
        defaultValue: ""
    }

    ConfigurationValue {
        id: strengthConfig
        key: "/apps/parrotfish-keyboard/autocorrect"   // off, gentle, normal
        defaultValue: "gentle"
    }

    onSelect: {
        if (showApplicationSuggestions) {
            MInputMethodQuick.sendCommit(text, -MInputMethodQuick.cursorPosition,
                                         MInputMethodQuick.surroundingText.length)
        } else {
            applyPrediction(text, index)
        }
    }

    onRemove: {
        // We're using an unused key modifier flag as a back channel.
        MInputMethodQuick.sendKey(Qt.Key_Delete, 0x80000000, text)
    }

    onPaste: {
        inputHandler.commit(inputHandler.preedit)
        MInputMethodQuick.sendCommit(Clipboard.text)
    }

    onActiveChanged: {
        if (!active && preedit !== "") {
            commit(preedit)
        }
        _correction = null
        _applySwipe()
        if (active) {
            _updateLayout()
            _contextChanged()
        }
    }

    onPreeditChanged: {
        if (preedit.length > 0 && _lastPreedit.length === 0) {
            // A new word: refresh the keys and the language context first.
            _updateLayout()
            _setContext(_textBefore())
        } else if (preedit.length === 0 && _lastPreedit.length > 0 && _committing) {
            // The word was committed. The app reports the new text a moment
            // later; until then, assume the word and a space.
            _setContext(_textBefore() + _lastPreedit + " ")
            _rememberLanguage()
        } else if (preedit.length === 0 && _lastPreedit.length > 0) {
            // The word was deleted letter by letter: only the text before it
            // counts (its last letter, say "i", is not a word typed).
            _setContext(_textBefore())
        }
        _lastPreedit = preedit
        predictionEngine.preedit = preedit
    }

    Connections {
        target: MInputMethodQuick
        onSurroundingTextChanged: inputHandler._contextChanged()
        onCursorPositionChanged: inputHandler._contextChanged()
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

    function applyPrediction(replacement, index) {
        replacement = replacement + " "
        candidateSpaceIndex = MInputMethodQuick.surroundingTextValid
                ? MInputMethodQuick.cursorPosition + replacement.length : -1
        commit(replacement)
    }

    function handleKeyClick() {
        var handled = false
        var correction = _correction
        _correction = null   // Backspace can only undo straight after
        keyboard.expandedPaste = false

        if (pressedKey.key === Qt.Key_Space) {
            if (preedit !== "") {
                _commitWord(" ")
                keyboard.autocaps = false // assuming no autocaps after input
            } else {
                commit(" ")
            }

            if (keyboard.shiftState !== ShiftState.LockedShift) {
                keyboard.shiftState = ShiftState.AutoShift
            }

            handled = true

        } else if (pressedKey.key === Qt.Key_Return) {
            if (preedit !== "") {
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
            predictionEngine.rejectCorrection(correction.typed)
            predictionEngine.log("undo: " + length + " characters replaced by "
                                 + (correction.typed.length + correction.suffix.length))
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
            if (isInputCharacter(pressedKey.text)) {
                preedit += _tappedText()

                if (keyboard.shiftState !== ShiftState.LockedShift) {
                    keyboard.shiftState = ShiftState.NoShift
                }

                MInputMethodQuick.sendPreedit(preedit)
                handled = true
            } else {
                // normal symbols etc.
                if (preedit !== "") {
                    if (".,;:!?)\"".indexOf(pressedKey.text) >= 0) {
                        _commitWord(pressedKey.text)
                    } else {
                        commit(preedit + pressedKey.text)
                    }
                } else {
                    if (candidateSpaceIndex > 0 && candidateSpaceIndex === MInputMethodQuick.cursorPosition
                            && ",.?!".indexOf(pressedKey.text) >= 0
                            && MInputMethodQuick.surroundingText.charAt(MInputMethodQuick.cursorPosition - 1) === " ") {
                        if (inputHandler.language === "FR" && "?!".indexOf(pressedKey.text) >= 0) {
                            // follow French grammar rules for ? and !
                            MInputMethodQuick.sendCommit(pressedKey.text + " ")
                        } else {
                            // replace automatically added space from candidate clicking
                            MInputMethodQuick.sendCommit(pressedKey.text + " ", -1, 1)
                        }
                        preedit = ""
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
            MInputMethodQuick.sendKey(Qt.Key_Backspace, 0, "\b", Maliit.KeyClick)
            MInputMethodQuick.sendPreedit(word, undefined, -wordLength, wordLength)
            preedit = word
            handled = true
        }

        if (pressedKey.keyType !== KeyType.ShiftKey && pressedKey.keyType !== KeyType.SymbolKey) {
            candidateSpaceIndex = -1
        }

        return handled
    }

    function isInputCharacter(character) {
        return KeyboardSupport.isLetter(character) || "\'-".indexOf(character) >= 0
    }

    function reset() {
        preedit = ""
        _correction = null
    }

    function commit(text) {
        _committing = true
        MInputMethodQuick.sendCommit(text)
        preedit = ""
        _committing = false
    }

    // Commits the word being typed, or its autocorrection, then `suffix`.
    function _commitWord(suffix) {
        var typed = preedit
        var word = predictionEngine.autocorrection !== "" ? predictionEngine.autocorrection : typed
        commit(word + suffix)
        if (word !== typed) {
            _correction = { "typed": typed, "corrected": word, "suffix": suffix }
        }
    }

    // Only if the correction is right before the cursor (the cursor may have
    // been moved by tapping the text, which is not a key click).
    function _canUndo(correction) {
        if (!MInputMethodQuick.surroundingTextValid)
            return true   // the app doesn't tell; Backspace came straight after
        var before = _textBefore()
        var expected = correction.corrected + correction.suffix
        return before.length >= expected.length
                && before.substr(before.length - expected.length) === expected
    }

    // The text of the tapped key; for a tap that started on a neighbouring
    // letter key and slid onto this one, the letter that fits the word better.
    function _tappedText() {
        var text = pressedKey.text
        var initial = keyboard.lastInitialKey
        // lastInitialKey belongs to the latest touch. When a second finger lands
        // before the first lifts, Jolla's keyboard types the first finger's key
        // while lastInitialKey is already the second finger's: then
        // lastPressedKey is the second finger's key too, not the key being typed.
        if (initial && initial !== pressedKey && keyboard.lastPressedKey === pressedKey
                && initial.keyType === pressedKey.keyType
                && typeof initial.text === "string" && initial.text.length === 1
                && text.length === 1 && isInputCharacter(initial.text)
                && _adjacent(initial, pressedKey)) {
            text = predictionEngine.resolveTap(preedit, text, initial.text)
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

    function _contextChanged() {
        if (active && preedit.length === 0)
            _setContext(_textBefore())
    }

    function _setContext(text) {
        predictionEngine.rememberedLanguage = lastLanguageConfig.value
        predictionEngine.setContext(text)
    }

    function _textBefore() {
        if (!MInputMethodQuick.surroundingTextValid)
            return ""
        var pos = MInputMethodQuick.cursorPosition
        return MInputMethodQuick.surroundingText.substring(Math.max(0, pos - 1000), pos)
    }

    // Letter keys and the apostrophe key: the keys words are typed with.
    function _isLetterKey(item) {
        return item.keyType !== undefined && typeof item.caption === "string"
                && item.caption.length === 1
                && (item.caption.toLowerCase() !== item.caption.toUpperCase() || item.caption === "'")
                && item.visible && item.width > 0
    }

    function _scan(item, root, keys) {
        var children = item.children
        if (!children)
            return
        for (var i = 0; i < children.length; ++i) {
            var child = children[i]
            if (_isLetterKey(child)) {
                var p = child.mapToItem(root, child.width / 2, child.height / 2)
                keys.push({ "ch": child.caption, "x": p.x, "y": p.y,
                            "width": child.width, "height": child.height })
            } else if (child.key === Qt.Key_Space && child.languageLabel !== undefined
                       && child.languageLabel !== "" && _spacebar === null) {
                _spacebar = child
                _spacebarLabel = _findLabel(child)
            } else {
                _scan(child, root, keys)
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

    function _updateLayout() {
        var layout = keyboard.layout
        if (!layout)
            return
        var revision = typeof layout.keysRevision === "number" ? layout.keysRevision : 0
        if (layout === _keysLayout && layout.width === _keysWidth && revision === _keysRevision)
            return
        var keys = []
        _spacebar = null
        _spacebarLabel = null
        try {
            _scan(layout, layout, keys)
        } catch (e) {
            predictionEngine.log("scanning the layout failed: " + e)
        }
        predictionEngine.log("layout " + layout.languageCode + ": " + keys.length
                             + " letter keys, spacebar " + (_spacebar !== null)
                             + ", spacebar label " + (_spacebarLabel !== null))
        if (predictionEngine.setKeys(keys) > 0) {
            _keysLayout = layout
            _keysWidth = layout.width
            _keysRevision = revision
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
        if (lead !== "")
            label = unsure && _spacebarLabel ? "<i>" + lead + "</i>" : lead
        if (_spacebar.languageLabel !== label)
            _spacebar.languageLabel = label
    }

    topItem: Component {
        TopItem {
            HorizontalPredictionListView {
                id: predictionList

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
                        visible: inputHandler.showDividers && model.index > 0
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

            VerticalPredictionListView {
                id: verticalList

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
                        visible: inputHandler.showDividers && model.index > 0
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
