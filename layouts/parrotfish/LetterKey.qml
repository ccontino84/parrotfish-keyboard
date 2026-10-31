// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Jolla's CharacterKey with a KeyBackground. With a background, the key has
// no separator line and is highlighted through its background when pressed.
// A LongPressHint marks the keys with a long-press menu. The sticky key
// (sticky: true; parrotfish.qml sets its sign) looks like a function key.
import QtQuick 2.0
import com.jolla.keyboard 1.0
import "../.."

CharacterKey {
    property bool sticky: false

    separator: keyBackground.styled ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator
    showHighlight: !keyBackground.styled

    KeyBackground {
        id: keyBackground
        functionLook: parent.sticky
    }
    LongPressHint { frame: keyBackground }
}
