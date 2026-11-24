// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LGPL-2.1-or-later
//
// The bottom row of the Parrotfish layouts. Based on SpacebarRow.qml of
// jolla-keyboard 0.10.4 (Jolla Ltd., BSD-3-Clause). Changes: comma and
// period with a KeyBackground, a LongPressHint and the typing languages'
// signs on long press (the comma's too); the layout's gap; the widths set
// by the layout; the mic glyph; the numpad's 0 and a sign after the
// spacebar; ЙЦУКЕН's - or ' after the symbol key, as on Jolla's ru.qml and
// uk.qml, with more signs on long press.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import Parrotfish.Keyboard 1.0
import "../.."
import "arrangements.js" as Arrangements

KeyboardRow {
    id: bottomRow

    splitIndex: 4   // the extra key counts, shown or not

    // The layout's gap instead of Jolla's paddingMedium, so that the rows line up.
    Component.onCompleted: {
        for (var i = 0; i < children.length; ++i) {
            var key = children[i]
            for (var j = 0; j < key.children.length; ++j) {
                var item = key.children[j]
                if (item.radius !== undefined && item.styled === undefined && item.anchors
                        && item.anchors.margins === Theme.paddingMedium)
                    item.anchors.margins = Qt.binding(function() {
                        return bottomRow.parent && bottomRow.parent.keyGap !== undefined
                                ? bottomRow.parent.keyGap : Theme.paddingMedium
                    })
            }
        }
    }

    readonly property var _punctuation: parent && parent.punctuation !== undefined ? parent.punctuation : null
    readonly property bool _numpad: !!parent && parent.numpadShown === true
    readonly property string _numpadBottom: Arrangements.numpad.bottom
    readonly property var _numpadMore: parent && parent.arrangementKey !== undefined
                                       ? Arrangements.numpadMore[parent.arrangementKey] || {} : {}
    readonly property bool _voice: !!parent && parent.voiceEnabled === true

    // Outside split landscape. Set when the layout sizes the rows: bindings
    // may not have followed the setting or the layout's width yet.
    function _setWidths() {
        var percent = parent && parent.spaceWidthPercent !== undefined ? parent.spaceWidthPercent : 100
        var punctuation = splitActive ? punctuationKeyWidth : width / 10
        var space = (width - 2 * (functionKeyWidth + punctuationKeyWidth)) * percent / 100
        // The numpad's 0 under its 8: the ends 1.5 keys wide, as 1/2 and Backspace.
        var end = splitActive ? functionKeyWidth : _numpad ? width / 10 * 1.5
                                                 : Math.max(shiftKeyWidth, (width - space) / 2 - punctuation)
        if (comma.implicitWidth !== punctuation)
            comma.implicitWidth = punctuation
        if (period.implicitWidth !== punctuation)
            period.implicitWidth = punctuation
        if (extra.implicitWidth !== punctuation)
            extra.implicitWidth = punctuation
        // With the extra key, ?123 is narrow, as on Jolla's ru.qml.
        var symbolWidth = extra.active && !splitActive ? symbolKeyWidthNarrow : end
        if (symbol.implicitWidth !== symbolWidth)
            symbol.implicitWidth = symbolWidth
        if (enter.implicitWidth !== end)
            enter.implicitWidth = end
        var keys = [afterSpace1, afterSpace2]
        for (var i = 0; i < keys.length; ++i)
            if (keys[i].implicitWidth !== punctuation)
                keys[i].implicitWidth = punctuation
    }
    // KeyboardRow's, with the widths set first: the layout reads them
    // before it calls relayout().
    function maximumBasicButtonWidth(shareableWidth) {
        _setWidths()
        if (avoidanceWidth > 0) {
            var split = splitIndex > 0 ? splitIndex : Math.floor(children.length / 2)
            shareableWidth = (shareableWidth - avoidanceWidth) / 2
            return Math.min(_maximumButtonWidthPart(shareableWidth, 0, split),
                            _maximumButtonWidthPart(shareableWidth, split, children.length))
        }
        return _maximumButtonWidthPart(shareableWidth, 0, children.length)
    }
    function relayout(basicButtonWidth) {
        _setWidths()
        if (avoidanceWidth > 1) {
            var shareableWidth = width - avoidanceWidth
            var split = splitIndex > 0 ? splitIndex : Math.floor(children.length / 2)
            _relayoutPart(basicButtonWidth, 0, split, 0, shareableWidth / 2)
            _relayoutPart(basicButtonWidth, split, children.length, width / 2 + avoidanceWidth / 2, shareableWidth / 2)
        } else {
            _relayoutPart(basicButtonWidth, 0, children.length, 0, width)
        }
    }

    SymbolKey {
        id: symbol
        symbolCaption: bottomRow.parent && bottomRow.parent.arrangement
                       && bottomRow.parent.arrangement.symbolCaption ? bottomRow.parent.arrangement.symbolCaption : "ABC"
    }
    LetterKey {
        id: extra
        readonly property var _layout: bottomRow.parent
        readonly property var _a: _layout && _layout.arrangement ? _layout.arrangement : null
        active: !bottomRow._numpad && _a !== null && _a.bottomSign !== undefined
        caption: active ? _a.bottomSign : ""
        captionShifted: caption
        accents: active ? Arrangements.signsAfter(caption) : ""
        accentsShifted: accents
        fixedWidth: !splitActive
    }
    ContextAwareCommaKey {
        id: comma
        property bool popupAlways: true   // as PeriodKey: also in the symbol view
        accents: caption !== "," ? "" : bottomRow._punctuation ? bottomRow._punctuation.comma : ":,;"
        accentsShifted: accents
        showHighlight: !commaBackground.styled
        KeyBackground { id: commaBackground }
        LongPressHint { frame: commaBackground }
    }
    SpacebarKey {
        id: space
        Icon {
            visible: bottomRow._voice
            anchors { right: parent.right; rightMargin: Theme.paddingLarge; verticalCenter: parent.verticalCenter }
            width: Math.round(parent.height * 0.42)
            height: width
            sourceSize.width: width
            sourceSize.height: height
            source: "image://theme/icon-m-mic"
            opacity: 0.6
        }
    }
    // The numpad's bottom keys.
    LetterKey {
        id: afterSpace1
        active: bottomRow._numpad
        caption: bottomRow._numpadBottom.charAt(0)
        captionShifted: caption
        accents: bottomRow._numpadMore[caption] || ""
        accentsShifted: accents
        popupAlways: accents !== ""
        fixedWidth: true
        pad: caption === "0"
    }
    LetterKey {
        id: afterSpace2
        active: afterSpace1.active
        caption: bottomRow._numpadBottom.charAt(1)
        captionShifted: caption
        fixedWidth: true
        pad: caption === "0"
    }
    SpacebarKey {
        active: splitActive
        languageLabel: ""
    }
    PeriodKey {
        id: period
        accents: bottomRow._punctuation ? bottomRow._punctuation.period : "!.?"
        accentsShifted: accents
        showHighlight: !periodBackground.styled
        KeyBackground { id: periodBackground }
        LongPressHint { frame: periodBackground }
    }
    EnterKey {
        id: enter
    }
}
