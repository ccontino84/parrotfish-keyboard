// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Layout: the keys, the number row, the sticky key, swiping. Opened from
// Parrotfish's section in Settings → Text input (ParrotfishSettings.qml),
// with the keyboard open to try the changes (KeyboardPreview.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "languages.js" as Languages

Page {
    id: page

    ConfigurationValue {
        id: stickyConfig
        key: "/apps/parrotfish-keyboard/sticky"
        defaultValue: true
    }

    ConfigurationValue {
        id: stickySideConfig
        key: "/apps/parrotfish-keyboard/sticky-side"
        defaultValue: "right"
    }

    ConfigurationValue {
        id: numberRowConfig
        key: "/apps/parrotfish-keyboard/number-row"
        defaultValue: true
    }

    ConfigurationValue {
        id: numberRowHeightConfig
        key: "/apps/parrotfish-keyboard/number-row-height"
        defaultValue: "75"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Layout")
            }

            KeyboardPreview {
                page: page
            }

            SettingCombo {
                key: "layout"
                label: qsTr("Layout")
                values: ["qwerty", "qwertz", "azerty", "nordic"]
                texts: ["QWERTY", "QWERTZ", "AZERTY", "QWERTY (ÅÖÄ)"]
                defaultValue: Languages.layoutFor(Qt.locale().name)
            }

            // Two keys, one choice: number-row (on, off) and its height.
            ComboBox {
                id: numberRowBox

                readonly property bool on: numberRowConfig.value !== false && numberRowConfig.value !== "false"
                readonly property bool smaller: String(numberRowHeightConfig.value) !== "100"

                width: parent.width
                label: qsTr("Number row")
                currentIndex: !on ? 0 : smaller ? 2 : 1
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Off")
                        onClicked: numberRowConfig.value = false
                    }
                    MenuItem {
                        text: qsTr("Normal")
                        onClicked: {
                            numberRowHeightConfig.value = "100"
                            numberRowConfig.value = true
                        }
                    }
                    MenuItem {
                        text: qsTr("Smaller")
                        onClicked: {
                            numberRowHeightConfig.value = "75"
                            numberRowConfig.value = true
                        }
                    }
                }
            }

            // Two keys, one choice: sticky (on, off) and sticky-side.
            ComboBox {
                readonly property bool on: stickyConfig.value !== false && stickyConfig.value !== "false"

                width: parent.width
                label: qsTr("Sticky key")
                description: qsTr("Keeps the last sign picked from its long press")
                currentIndex: !on ? 0 : stickySideConfig.value === "left" ? 2 : 1
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Off")
                        onClicked: stickyConfig.value = false
                    }
                    MenuItem {
                        text: qsTr("Right")
                        onClicked: {
                            stickySideConfig.value = "right"
                            stickyConfig.value = true
                        }
                    }
                    MenuItem {
                        text: qsTr("Left")
                        onClicked: {
                            stickySideConfig.value = "left"
                            stickyConfig.value = true
                        }
                    }
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
