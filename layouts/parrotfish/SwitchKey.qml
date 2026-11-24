// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The key that shows the next typing language's keys (ЙЦУКЕН: Russian,
// Ukrainian), a language icon with the look of a function key; the spacebar
// shows the language. Types nothing; symView makes Jolla's KeyboardRow size
// it as a letter.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../.."

CharacterKey {
    id: switchKey

    property Item layout
    // The language whose keys a tap shows.
    property string target

    keyType: KeyType.FunctionKey
    captionShifted: caption
    symView: caption
    symView2: caption
    keyText: ""
    text: ""
    showPopper: false
    showHighlight: !keyBackground.shown
    separator: keyBackground.shown ? SeparatorState.HiddenSeparator : SeparatorState.AutomaticSeparator

    onClicked: if (layout) layout.switchKeys(target)

    KeyBackground {
        id: keyBackground
        functionLook: true
    }

    Icon {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: Math.round((switchKey.leftPadding - switchKey.rightPadding) / 2)
        readonly property int side: Math.round(Math.min(switchKey.width - switchKey.leftPadding - switchKey.rightPadding,
                                                        switchKey.height) * 0.7)
        width: side
        height: side
        sourceSize.width: side
        sourceSize.height: side
        source: "image://theme/icon-m-region"
        opacity: switchKey.pressed ? 0.6 : 1.0
    }
}
