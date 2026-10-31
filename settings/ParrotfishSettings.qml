// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Parrotfish's section in Settings → Text input (the typing languages
// have their own page, ParrotfishLanguages.qml). Jolla's text input page
// loads it (a Loader per settings= page of the enabled layouts' handlers),
// so it's a plain Column; Jolla shows it while the Parrotfish layout is
// enabled. Every value is a dconf key the keyboard watches: changes apply at
// once.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "languages.js" as Languages

Column {
    SectionHeader {
        text: "Parrotfish"
    }

    ValueButton {
        label: qsTr("Typing languages")
        value: Languages.names(Languages.selected(languagesConfig.value, Qt.locale().name))
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLanguages.qml"))

        ConfigurationValue {
            id: languagesConfig
            key: "/apps/parrotfish-keyboard/languages"
            defaultValue: ""
        }
    }

    // A ComboBox for one setting of a Repeater model (see below).
    Component {
        id: settingBox

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
                defaultValue: comboBox.setting["default"] !== undefined ? comboBox.setting["default"]
                                                                        : comboBox.setting.values[0]
            }
        }
    }

    Repeater {
        // key: under /apps/parrotfish-keyboard/; the default is "default", or
        // else the first value
        model: [
            { "key": "layout", "label": qsTr("Layout"), "description": "",
              "values": ["qwerty", "qwertz", "azerty", "nordic"],
              "default": Languages.layoutFor(Qt.locale().name),
              "texts": ["QWERTY", "QWERTZ", "AZERTY", "QWERTY ÅÖÄ"] },
            { "key": "autocorrect", "label": qsTr("Autocorrect"), "description": "",
              "values": ["strong", "normal", "gentle", "off"], "default": "normal",
              "texts": [qsTr("Strong (experimental)"), qsTr("Normal"), qsTr("Gentle"), qsTr("Off")] },
            { "key": "key-height", "label": qsTr("Key height"),
              "description": qsTr("In portrait"),
              "values": ["100", "90", "80"],
              "texts": ["100%", "90%", "80%"] },
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
              "texts": [qsTr("None"), qsTr("Line"), qsTr("Raised")] },
            { "key": "sticky-side", "label": qsTr("Sticky key"),
              "description": qsTr("Keeps the last sign picked from its long press"),
              "values": ["right", "left"],
              "texts": [qsTr("On the right"), qsTr("On the left")] },
            { "key": "key-gap", "label": qsTr("Space between keys"),
              "description": qsTr("Share of the key height"),
              "values": ["2", "3", "5", "7", "10"], "default": "5",
              "texts": ["2%", "3%", "5%", "7%", "10%"] },
            { "key": "long-press-hint", "label": qsTr("Long-press hint"),
              "description": qsTr("Marks keys with more characters"),
              "values": ["dot", "line", "off"], "default": "dot",
              "texts": [qsTr("Dot"), qsTr("Line"), qsTr("Off")] }
        ]

        delegate: settingBox
    }

    TextSwitch {
        automaticCheck: false
        checked: numberRowConfig.value === true || numberRowConfig.value === "true"
        text: qsTr("Number row")
        onClicked: numberRowConfig.value = !checked

        ConfigurationValue {
            id: numberRowConfig
            key: "/apps/parrotfish-keyboard/number-row"
            defaultValue: false
        }
    }

    TextSwitch {
        visible: numberRowConfig.value === true || numberRowConfig.value === "true"
        automaticCheck: false
        checked: String(numberRowHeightConfig.value) !== "100"
        text: qsTr("Shorter number row")
        onClicked: numberRowHeightConfig.value = checked ? "100" : "75"

        ConfigurationValue {
            id: numberRowHeightConfig
            key: "/apps/parrotfish-keyboard/number-row-height"
            defaultValue: "100"
        }
    }

    Column {
        width: parent.width
        visible: numberRowConfig.value === true || numberRowConfig.value === "true"

        Repeater {
            model: [
                { "key": "number-row-fill", "label": qsTr("Number row fill"), "description": "",
                  "values": ["same", "none", "flat", "shaded", "band"],
                  "texts": [qsTr("As the keys"), qsTr("None"), qsTr("Flat"), qsTr("Shaded"), qsTr("One band")] },
                { "key": "number-row-border", "label": qsTr("Number row border"), "description": "",
                  "values": ["same", "none", "line", "raised"],
                  "texts": [qsTr("As the keys"), qsTr("None"), qsTr("Line"), qsTr("Raised")] },
                { "key": "number-row-digits", "label": qsTr("Number row digits"), "description": "",
                  "values": ["normal", "quiet"],
                  "texts": [qsTr("Normal"), qsTr("Smaller and fainter")] }
            ]
            delegate: settingBox
        }
    }

    TextSwitch {
        automaticCheck: false
        checked: spaceBeforeConfig.value === true || spaceBeforeConfig.value === "true"
        text: qsTr("Space before ! ? : ;")
        description: qsTr("Keeps the space after a picked suggestion, as in French")
        onClicked: spaceBeforeConfig.value = !checked

        ConfigurationValue {
            id: spaceBeforeConfig
            key: "/apps/parrotfish-keyboard/space-before-punctuation"
            defaultValue: false
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
