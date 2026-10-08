// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Jolla's CharacterKey with a KeyBackground. With a background, the key has
// no separator line and is highlighted through its background when pressed.
import QtQuick 2.0
import com.jolla.keyboard 1.0
import "../.."

CharacterKey {
    separator: keyBackground.styled ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator
    showHighlight: !keyBackground.styled

    KeyBackground { id: keyBackground }
}
