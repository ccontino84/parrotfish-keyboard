// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A field to try the keyboard in, focused when the page (`page`) opens:
// Qt.inputMethod.show() alone does nothing on the phone. Its
// __inputMethodExtensions carry parrotfishNoLearning; a plain TextInput, as
// the focused item must carry them (a TextField's is its inner editor).
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: previewItem

    property Item page

    x: Theme.horizontalPageMargin
    width: (parent ? parent.width : 0) - 2 * x
    height: hint.y + hint.height + Theme.paddingMedium

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

    // What the field doesn't keep (the handler: no learning, no recent emoji).
    Label {
        id: hint

        y: preview.y + preview.height + Theme.paddingSmall
        width: parent.width
        wrapMode: Text.Wrap
        color: Theme.secondaryHighlightColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: qsTr("Words and emoji typed here aren't remembered")
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
            // The word being typed isn't in `text`, and a reset leaves it
            // shown: commit it first (TextInput.clear() does both from Qt 5.7).
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
