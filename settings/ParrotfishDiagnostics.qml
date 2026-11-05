// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// What a bug report needs, to copy (KeyboardInfo::diagnostics(): no word
// the user typed). Opened by a double tap on Parrotfish's section header
// (ParrotfishSettings.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property QtObject info
    readonly property string text: info ? info.diagnostics() : ""

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Diagnostics")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                font.family: "monospace"
                color: Theme.highlightColor
                text: page.text
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Copy")
                enabled: page.text !== ""
                onClicked: Clipboard.text = page.text
            }
        }

        VerticalScrollDecorator {}
    }
}
