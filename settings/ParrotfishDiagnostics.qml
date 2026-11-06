// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// What a bug report needs (KeyboardInfo::diagnostics(): no word the user
// typed), to copy or to send with Report a bug. Opened by a double tap on
// the version in Parrotfish's section (ParrotfishSettings.qml).
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

            Row {
                id: buttons

                readonly property real buttonWidth: Math.min(Theme.buttonWidthMedium,
                                                             (column.width - 2 * Theme.horizontalPageMargin - spacing) / 2)

                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge

                Button {
                    width: buttons.buttonWidth
                    text: qsTr("Copy")
                    enabled: page.text !== ""
                    onClicked: Clipboard.text = page.text
                }

                Button {
                    width: buttons.buttonWidth
                    text: qsTr("Report a bug")
                    // A new issue in the browser, from the bug report form
                    // (.github/ISSUE_TEMPLATE/bug_report.yml), the diagnostics filled in.
                    onClicked: Qt.openUrlExternally("https://github.com/ccontino84/parrotfish-keyboard/issues/new"
                                                    + "?template=bug_report.yml&diagnostics="
                                                    + encodeURIComponent(page.text))
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
