// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Layout: which keys the keyboard has (the number row, the language keys,
// the sticky key). Opened from Parrotfish's section in Settings → Text
// input (ParrotfishSettings.qml), with the keyboard open to try the
// changes (KeyboardPreview.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0
import Nemo.Configuration 1.0

Page {
    id: page

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

    ConfigurationValue {
        id: languageKeysConfig
        key: "/apps/parrotfish-keyboard/language-keys"
        defaultValue: false
    }

    ConfigurationValue {
        id: languageKeysSideConfig
        key: "/apps/parrotfish-keyboard/language-keys-side"
        defaultValue: "right"
    }

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

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
                        text: qsTr("Off", "number row")
                        onClicked: numberRowConfig.value = false
                    }
                    MenuItem {
                        text: qsTr("Normal", "number row")
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

            // Two keys, one choice: language-keys (on, off) and
            // language-keys-side. The keys (ParrotfishKeyboard.qml) are shown
            // only if a typing language has such letters.
            ComboBox {
                readonly property bool on: String(languageKeysConfig.value) === "true"
                readonly property bool anyLetters: {
                    var codes = Languages.selected(languagesConfig.value)
                    for (var i = 0; i < codes.length; ++i)
                        if (Languages.extraKeys(codes[i]) !== "")
                            return true
                    return false
                }

                width: parent.width
                label: qsTr("Language keys")
                description: on && !anyLetters ? qsTr("None of your typing languages uses them")
                                               : qsTr("Accented letters that follow the language you type")
                currentIndex: !on ? 0 : languageKeysSideConfig.value === "left" ? 2 : 1
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Off", "language keys")
                        onClicked: languageKeysConfig.value = false
                    }
                    MenuItem {
                        text: qsTr("Right")
                        onClicked: {
                            languageKeysSideConfig.value = "right"
                            languageKeysConfig.value = true
                        }
                    }
                    MenuItem {
                        text: qsTr("Left")
                        onClicked: {
                            languageKeysSideConfig.value = "left"
                            languageKeysConfig.value = true
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
                        text: qsTr("Off", "sticky key")
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
        }

        VerticalScrollDecorator {}
    }
}
