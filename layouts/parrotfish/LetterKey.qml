// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Jolla's CharacterKey with a KeyBackground. With a background, the key has
// no separator line and is highlighted through its background when pressed.
// A LongPressHint marks the keys with a long-press menu. The sticky key
// (sticky: true; parrotfish.qml sets its sign) looks like a function key,
// in the letter view only. moreAccents (moreAccentsShifted): the letters
// behind › in its long-press menu (LongPressMenu.qml). numberRow: a key of
// the number row, with the row's look (KeyBackground) and, if the layout's
// numberDigits is "quiet", a smaller and fainter digit.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../.."

CharacterKey {
    property bool sticky: false
    property string moreAccents
    property string moreAccentsShifted
    property bool numberRow: false
    readonly property bool _quiet: numberRow && keyBackground.layout !== null
                                   && keyBackground.layout.numberDigits === "quiet"
    // In the symbol view the sticky key types a symbol like its neighbours:
    // it looks like them.
    readonly property var _attributes: keyBackground.layout && keyBackground.layout.attributes
                                       ? keyBackground.layout.attributes : null
    readonly property bool _symbols: _attributes !== null && (_attributes.inSymView || _attributes.inSymView2)

    // No separators on a background, nor on the number row's band.
    readonly property bool _onBand: numberRow && keyBackground.layout !== null
                                    && keyBackground.layout.numberFill === "band"
    separator: keyBackground.styled || _onBand ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator
    showHighlight: !keyBackground.styled
    pixelSize: _quiet ? Math.round(Theme.fontSizeLarge * 0.8) : Theme.fontSizeLarge

    // CharacterKey's label has no colour property to set: it is faded.
    Component.onCompleted: {
        for (var i = 0; i < children.length; ++i) {
            var c = children[i]
            if (c.font !== undefined && c.text !== undefined && c.horizontalAlignment !== undefined)
                c.opacity = Qt.binding(function() { return _quiet ? 0.6 : 1 })
        }
    }

    KeyBackground {
        id: keyBackground
        functionLook: parent.sticky && !parent._symbols
        numberRow: parent.numberRow
    }
    LongPressHint { frame: keyBackground }
}
