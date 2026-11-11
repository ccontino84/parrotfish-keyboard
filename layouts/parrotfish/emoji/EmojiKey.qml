// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A key of the emoji panel. The emoji is drawn by the plugin
// (image://parrotfish-emoji): Qt 5.6 draws emoji in black and white, which
// is also why it has no popper. `base`: the emoji in the panel's list,
// `emoji` its picked variant; `removable`: a recent emoji.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../../.."
import Parrotfish.Keyboard 1.0

CharacterKey {
    id: emojiKey

    property string emoji
    property string base: emoji
    property string variants
    property bool removable: false
    property bool longPressHint: true

    caption: emoji
    captionShifted: emoji
    keyText: ""
    text: emoji
    showPopper: false
    separator: SeparatorState.HiddenSeparator
    enabled: emoji !== ""

    Image {
        id: image
        anchors.centerIn: parent
        readonly property int side: Math.round(Math.min(emojiKey.width, emojiKey.height) * 0.8)
        width: side
        height: side
        sourceSize.width: side
        sourceSize.height: side
        asynchronous: true
        source: side > 0 ? EmojiUtil.source(emojiKey.emoji) : ""
    }

    // The long-press mark, as LongPressHint's dot, at the emoji's corner.
    Rectangle {
        readonly property int thickness: Math.max(2, Math.round(3.5 * Theme.pixelRatio))
        visible: emojiKey.variants !== "" && !emojiKey.pressed
                 && emojiKey.longPressHint
        width: thickness
        height: thickness
        radius: thickness / 2
        color: Theme.rgba(emojiKey.palette.primaryColor, 0.4)
        x: image.x + image.width - width / 2
        y: image.y - height / 2
    }
}
