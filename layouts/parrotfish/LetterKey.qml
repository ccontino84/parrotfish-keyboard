// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Jolla's CharacterKey with a KeyBackground. With a background, the key has
// no separator line and is highlighted through its background when pressed.
// A LongPressHint marks the keys with a long-press menu. The sticky key
// (sticky: true; parrotfish.qml sets its sign) looks like a function key,
// in the letter view only. moreAccents (moreAccentsShifted): the letters
// behind › in its long-press menu (LongPressMenu.qml). numberRow: a key of
// the number row; in a shorter row (the layout's numberRowPercent below
// 100) its digit is smaller too. group: a language key (its letter follows
// the typing language), tinted (KeyBackground's groupLook).
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
    readonly property bool _small: numberRow && keyBackground.layout !== null
                                   && keyBackground.layout.numberRowPercent < 100
    // In the symbol view the sticky key types a symbol like its neighbours:
    // it looks like them.
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
        groupLook: parent.group && !parent._symbols
    }
    LongPressHint { frame: keyBackground }
}
