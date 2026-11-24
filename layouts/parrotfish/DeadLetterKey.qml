// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Jolla's DeadKey (Greek tonos: the next letter key types its
// deadKeyAccents letter) with the look of a function key; the properties
// the layout sets on every letter key.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../.."

DeadKey {
    property bool sticky: false
    property bool numberRow: false
    property bool group: false
    property string moreAccents
    property string moreAccentsShifted
    property string deadKeyAccents
    property string deadKeyAccentsShifted

    readonly property var _attributes: keyBackground.layout && keyBackground.layout.attributes
                                       ? keyBackground.layout.attributes : null

    separator: keyBackground.shown ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator
    showHighlight: !keyBackground.shown

    KeyBackground {
        id: keyBackground
        // In the symbol view it types ΄ as a sign: a letter key's look there.
        functionLook: !(parent._attributes && (parent._attributes.inSymView || parent._attributes.inSymView2))
    }
}
