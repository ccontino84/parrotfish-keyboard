// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Look & feel: the keys' look and sizes, the number row, the sticky key,
// swiping (our layouts are switched on in Jolla's Keyboards page, as any
// other). Opened from
// Parrotfish's section in Settings → Text input (ParrotfishSettings.qml),
// with the keyboard open to try the changes (KeyboardPreview.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0
import Nemo.Configuration 1.0

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
                title: qsTr("Look & feel")
            }

            KeyboardPreview {
                page: page
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
