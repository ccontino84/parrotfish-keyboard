// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Typing aids, opened from the Typing page.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0

Page {
    id: page

    readonly property bool french: Languages.selectedAll(languagesConfig.value).indexOf("fr") >= 0

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

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
                key: "automatic-capitals"
                defaultValue: true
                text: qsTr("Automatic capitals")
                description: qsTr("A capital letter at the start of a sentence")
            }

            SettingSwitch {
                key: "double-space-period"
                defaultValue: true
                text: qsTr("Period on double space")
                description: qsTr("Space twice after a word types a period")
            }

            // The handler applies it to French text only.
            SettingSwitch {
                key: "space-before-punctuation"
                enabled: page.french
                text: qsTr("Space before ! ? : ;")
                description: page.french ? qsTr("In French text, keeps the space after a picked suggestion")
                                         : qsTr("Only with French as a typing language")
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
