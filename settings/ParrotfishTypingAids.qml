// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Typing aids: the on-off helpers for typing, opened from the Typing page
// (ParrotfishTyping.qml, which shows how many are on).
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Typing aids")
            }

            SettingSwitch {
                key: "double-space-period"
                defaultValue: true
                text: qsTr("Period on double space")
                description: qsTr("Space twice after a word types a period")
            }

            SettingSwitch {
                key: "space-before-punctuation"
                text: qsTr("Space before ! ? : ;")
                description: qsTr("Keeps the space after a picked suggestion, as in French")
            }

            SettingSwitch {
                key: "replace-emoticons"
                text: qsTr("Replace emoticons")
                description: qsTr("Space turns :-) into 🙂. Otherwise the emoji button offers it")
            }
        }

        VerticalScrollDecorator {}
    }
}
