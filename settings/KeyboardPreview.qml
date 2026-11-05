// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A field to try the keyboard in, at the top of a settings page (`page`):
// the keyboard follows the dconf keys at once, so the page shows it while
// it is open: the field takes the focus when the page opens (a keyboard
// without a focused field doesn't open: Qt.inputMethod.show() alone did
// nothing on the phone, alpha110).
// The keyboard doesn't learn from it: its __inputMethodExtensions, which
// Maliit passes on from the focused item, carry parrotfishNoLearning (the
// handler pauses learning). A plain TextInput, not a TextField: the focused
// item must carry the flag, and a TextField's is its inner editor.
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: previewItem

    property Item page

    x: Theme.horizontalPageMargin
    width: (parent ? parent.width : 0) - 2 * x
    height: preview.height + 2 * Theme.paddingMedium

    Connections {
        target: previewItem.page
        onStatusChanged: {
            if (previewItem.page.status === PageStatus.Active)
                preview.forceActiveFocus()
            else if (previewItem.page.status === PageStatus.Deactivating)
                preview.focus = false
        }
    }

    TextInput {
        id: preview

        property var __inputMethodExtensions: ({ "parrotfishNoLearning": true })

        y: Theme.paddingMedium
        width: parent.width
        color: Theme.highlightColor
        selectionColor: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        selectedTextColor: Theme.highlightColor
        font.pixelSize: Theme.fontSizeMedium
        font.family: Theme.fontFamily
        clip: true

        Label {
            visible: preview.text === "" && !preview.inputMethodComposing
            text: qsTr("Try the keyboard here")
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeMedium
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Math.max(1, Math.round(Theme.pixelRatio))
        color: preview.activeFocus ? Theme.highlightColor : Theme.secondaryHighlightColor
        opacity: 0.5
    }
}
