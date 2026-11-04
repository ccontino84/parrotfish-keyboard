// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Look & feel: key sizes and looks. Opened from Parrotfish's section in
// Settings → Text input (ParrotfishSettings.qml), with the keyboard open
// to try the changes (KeyboardPreview.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Look & feel")
            }

            KeyboardPreview {
                page: page
            }

            SettingSlider {
                key: "key-height"
                label: qsTr("Key height in portrait")
                minimumValue: 80
                maximumValue: 100
                stepSize: 5
                defaultValue: 85
            }

            SettingSlider {
                key: "key-height-landscape"
                label: qsTr("Key height in landscape")
                minimumValue: 80
                maximumValue: 100
                stepSize: 5
                defaultValue: 100
            }

            SettingSlider {
                key: "key-gap"
                label: qsTr("Space between keys")
                minimumValue: 0
                maximumValue: 10
                defaultValue: 5
            }

            SettingCombo {
                key: "key-fill"
                label: qsTr("Key fill")
                values: ["flat", "shaded", "none"]
                texts: [qsTr("Flat"), qsTr("Shaded"), qsTr("None")]
            }

            SettingCombo {
                key: "key-border"
                label: qsTr("Key border")
                values: ["line", "raised", "none"]
                texts: [qsTr("Line"), qsTr("Raised"), qsTr("None")]
            }

            TextSwitch {
                automaticCheck: false
                text: qsTr("Long-press hint")
                description: qsTr("A dot on keys with more characters")
                checked: hintConfig.value !== "off"
                onClicked: hintConfig.value = checked ? "off" : "dot"

                ConfigurationValue {
                    id: hintConfig
                    key: "/apps/parrotfish-keyboard/long-press-hint"
                    defaultValue: "dot"
                }
            }

            SettingSwitch {
                key: "dividers"
                defaultValue: true
                text: qsTr("Dividers between suggestions")
            }
        }

        VerticalScrollDecorator {}
    }
}
