// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Jolla's CharacterKey with a KeyBackground and a LongPressHint.
// moreAccents(Shifted): the letters behind › (LongPressMenu.qml); numberRow:
// a key of the number row; group: a language key; popupAlways: its
// long press works in the symbol view too (as Jolla's PeriodKey); pad: a
// numpad digit, with a language key's look.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../.."

CharacterKey {
    property bool sticky: false
    property string moreAccents
    property string moreAccentsShifted
    property bool numberRow: false
    property bool group: false
    property bool popupAlways: false
    property bool pad: false
    readonly property bool _small: numberRow && keyBackground.layout !== null
                                   && keyBackground.layout.numberRowPercent < 100
    // The sticky key looks like a function key in the letter view only.
    readonly property var _attributes: keyBackground.layout && keyBackground.layout.attributes
                                       ? keyBackground.layout.attributes : null
    readonly property bool _symbols: _attributes !== null && (_attributes.inSymView || _attributes.inSymView2)

    // No separators on a background.
    separator: keyBackground.shown ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator
    showHighlight: !keyBackground.shown
    pixelSize: _small ? Math.round(Theme.fontSizeLarge * 0.8) : Theme.fontSizeLarge

    KeyBackground {
        id: keyBackground
        functionLook: parent.sticky && !parent._symbols
        groupLook: (parent.group && !parent._symbols) || parent.pad
    }
    LongPressHint { frame: keyBackground }
}
