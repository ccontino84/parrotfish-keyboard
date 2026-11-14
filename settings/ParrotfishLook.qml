// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Look & feel: the keys' look and sizes, the long-press hint, swiping
// (our layouts are switched on in Jolla's Keyboards page, as any other).
// Opened from Parrotfish's section in Settings → Text input
// (ParrotfishSettings.qml), with the keyboard open to try the changes
// (KeyboardPreview.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0
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

            SettingCombo {
                key: "key-fill"
                label: qsTr("Key fill")
                values: ["none", "flat", "shaded"]
                texts: [qsTr("None", "key fill"), qsTr("Flat"), qsTr("Shaded")]
            }

            SettingCombo {
                key: "key-border"
                label: qsTr("Key border")
                values: ["none", "line", "raised"]
                texts: [qsTr("None", "key border"), qsTr("Line"), qsTr("Raised")]
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
                key: "space-width"
                label: qsTr("Space key width")
                minimumValue: 82
                maximumValue: 118
                stepSize: 6
                defaultValue: 100
            }

            SettingSlider {
                key: "key-gap"
                label: qsTr("Space between keys")
                minimumValue: 0
                maximumValue: 10
                defaultValue: 5
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

            // dconf swipe: "normal" or "off" (anything else: on).
            TextSwitch {
                automaticCheck: false
                text: qsTr("Swipe to switch keyboards")
                description: qsTr("Long-press the spacebar to switch too")
                checked: swipeConfig.value !== "off"
                onClicked: swipeConfig.value = swipeConfig.value === "off" ? "normal" : "off"

                ConfigurationValue {
                    id: swipeConfig
                    key: "/apps/parrotfish-keyboard/swipe"
                    defaultValue: "normal"
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
