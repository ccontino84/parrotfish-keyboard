// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The emoji button of split landscape, which has no suggestion bar: a key
// in the bottom letter row, doing what the handler's button does. It has
// symView, so that Jolla's KeyboardRow sizes it as a letter, and types
// nothing.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../../.."
import Parrotfish.Keyboard 1.0

CharacterKey {
    id: emojiKey

    property Item layout
    readonly property var handler: typeof keyboard !== "undefined" && keyboard.inputHandler
                                   && keyboard.inputHandler.emoticonBefore !== undefined
                                   ? keyboard.inputHandler : null
    readonly property string offer: handler && handler.emoticonBefore ? handler.emoticonBefore.emoji : ""
    property bool _held: false

    active: layout !== null && layout.splitActive
    keyType: KeyType.FunctionKey
    caption: ""
    captionShifted: ""
    keyText: ""
    text: ""
    showPopper: false
    showHighlight: false
    separator: SeparatorState.HiddenSeparator
    opacity: pressed ? 0.6 : 1.0

    onPressedChanged: {
        if (pressed) {
            _held = false
            hold.restart()
        } else {
            hold.stop()
        }
    }
    onClicked: {
        if (_held || layout === null)
            return
        if (offer !== "")
            handler.replaceEmoticon()
        else
            layout.emojiView = true
    }

    Timer {
        id: hold
        interval: 500   // as Jolla's long press
        onTriggered: {
            emojiKey._held = true
            if (emojiKey.layout)
                emojiKey.layout.emojiView = true
        }
    }

    // The offered emoji's highlight: a key's background (KeyBackground).
    Rectangle {
        readonly property real _gap: emojiKey.layout && emojiKey.layout.keyGap !== undefined
                                     ? emojiKey.layout.keyGap : -1
        anchors {
            fill: parent
            leftMargin: emojiKey.leftPadding + (_gap >= 0 ? _gap : Theme.paddingSmall)
            rightMargin: emojiKey.rightPadding + (_gap >= 0 ? _gap : Theme.paddingSmall)
            topMargin: _gap >= 0 ? _gap : Theme.paddingMedium
            bottomMargin: _gap >= 0 ? _gap : Theme.paddingMedium
        }
        radius: geometry.keyRadius
        color: Theme.highlightBackgroundColor
        opacity: Theme.highlightBackgroundOpacity
        visible: emojiKey.offer !== ""
    }

    Image {
        id: image
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: Math.round((emojiKey.leftPadding - emojiKey.rightPadding) / 2)
        readonly property int side: Math.round(Math.min(emojiKey.width - emojiKey.leftPadding - emojiKey.rightPadding,
                                                        emojiKey.height) * 0.55)
        width: side
        height: side
        sourceSize.width: side
        sourceSize.height: side
        source: !emojiKey.active || side <= 0 ? ""
                : EmojiUtil.source(emojiKey.offer !== "" ? emojiKey.offer : "\uD83D\uDE42")
    }
}
