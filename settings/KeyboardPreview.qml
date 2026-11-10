// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A field to try the keyboard in, at the top of a settings page (`page`):
// the keyboard follows the dconf keys at once, so the page shows it while
// it is open: the field takes the focus when the page opens (a keyboard
// without a focused field doesn't open: Qt.inputMethod.show() alone does
// nothing on the phone).
// The keyboard doesn't learn from it: its __inputMethodExtensions, which
// Maliit passes on from the focused item, carry parrotfishNoLearning (the
// handler pauses learning). A plain TextInput, not a TextField: the focused
// item must carry the flag, and a TextField's is its inner editor.
// A clear button as Silica's SearchField has (its icon, at the right while
// there is text): the word being typed is committed first, then the whole
// text cleared; the field keeps the focus so that the keyboard stays.
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
        width: parent.width - (clearButton.visible ? clearButton.width : 0)
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

    IconButton {
        id: clearButton

        anchors {
            right: parent.right
            rightMargin: -Theme.paddingMedium
            verticalCenter: preview.verticalCenter
        }
        visible: preview.text !== "" || preview.inputMethodComposing
        icon.source: "image://theme/icon-m-input-clear"
        onClicked: {
            // A word being typed isn't in `text` yet, and a reset only
            // tells the keyboard: the field kept showing it until the next
            // key (seen on the phone). Committed, it is in the text (at
            // once: maliit sends it to the field before returning), and
            // goes with it. TextInput.clear() does both, from Qt 5.7.
            Qt.inputMethod.commit()
            preview.text = ""
            preview.forceActiveFocus()
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
