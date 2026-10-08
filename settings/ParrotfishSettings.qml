// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Parrotfish's section in Settings → Text input. Jolla's text input page
// loads it (a Loader per settings= page of the enabled layouts' handlers),
// so it's a plain Column; Jolla shows it while the Parrotfish layout is
// enabled. Every value is a dconf key the keyboard watches: changes apply at
// once.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Column {
    SectionHeader {
        text: "Parrotfish"
    }

    Repeater {
        // key: under /apps/parrotfish-keyboard/; the first value is the default
        model: [
            { "key": "layout", "label": qsTr("Layout"), "description": "",
              "values": ["qwerty", "qwertz"],
              "texts": ["QWERTY", "QWERTZ"] },
            { "key": "autocorrect", "label": qsTr("Autocorrect"), "description": "",
              "values": ["gentle", "normal", "off"],
              "texts": [qsTr("Gentle"), qsTr("Normal"), qsTr("Off")] },
            { "key": "swipe", "label": qsTr("Swipe between keyboards"),
              "description": qsTr("Off: long-press the spacebar to switch"),
              "values": ["normal", "off"],
              "texts": [qsTr("Normal"), qsTr("Off")] },
            { "key": "key-fill", "label": qsTr("Key fill"),
              "description": "",
              "values": ["none", "flat", "shaded"],
              "texts": [qsTr("None"), qsTr("Flat"), qsTr("Shaded")] },
            { "key": "key-border", "label": qsTr("Key border"),
              "description": "",
              "values": ["none", "line", "raised"],
              "texts": [qsTr("None"), qsTr("Line"), qsTr("Raised")] }
        ]

        ComboBox {
            id: comboBox

            readonly property var setting: modelData

            width: parent.width
            label: setting.label
            description: setting.description
            currentIndex: Math.max(0, setting.values.indexOf(config.value))

            menu: ContextMenu {
                Repeater {
                    model: comboBox.setting.texts
                    MenuItem {
                        text: modelData
                        onClicked: config.value = comboBox.setting.values[index]
                    }
                }
            }

            ConfigurationValue {
                id: config
                key: "/apps/parrotfish-keyboard/" + comboBox.setting.key
                defaultValue: comboBox.setting.values[0]
            }
        }
    }

    TextSwitch {
        automaticCheck: false
        checked: dividersConfig.value !== false && dividersConfig.value !== "false"
        text: qsTr("Dividers between suggestions")
        onClicked: dividersConfig.value = !checked

        ConfigurationValue {
            id: dividersConfig
            key: "/apps/parrotfish-keyboard/dividers"
            defaultValue: true
        }
    }
}
