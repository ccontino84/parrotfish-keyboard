// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Typing: languages, autocorrect, learning, typing aids. Opened from Parrotfish's
// section in Settings → Text input (ParrotfishSettings.qml), with the
// keyboard open to try the languages and autocorrect (KeyboardPreview.qml:
// it learns nothing there).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0

Page {
    id: page

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

    LearnedWords {
        id: learnedWords
    }

    // The typing aids (ParrotfishTypingAids.qml), for how many are on.
    ConfigurationValue {
        id: doubleSpacePeriodConfig
        key: "/apps/parrotfish-keyboard/double-space-period"
    }
    ConfigurationValue {
        id: spaceBeforeConfig
        key: "/apps/parrotfish-keyboard/space-before-punctuation"
    }
    ConfigurationValue {
        id: replaceEmoticonsConfig
        key: "/apps/parrotfish-keyboard/replace-emoticons"
    }
    // As SettingSwitch reads them: unset is `defaultValue`.
    function _on(value, defaultValue) {
        return value === undefined || value === null || value === "" ? defaultValue
                                                                     : value === true || value === "true"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Typing")
            }

            KeyboardPreview {
                page: page
            }

            ValueButton {
                label: qsTr("Typing languages")
                value: Languages.names(Languages.selected(languagesConfig.value))
                       || qsTr("No languages installed")
                onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLanguages.qml"))
            }

            SettingCombo {
                key: "autocorrect"
                label: qsTr("Autocorrect")
                values: ["strong", "normal", "gentle", "off"]
                defaultValue: "normal"
                // The context of Off, Normal, None: their gender in some languages.
                texts: [qsTr("Strong"), qsTr("Normal", "autocorrect"), qsTr("Gentle"), qsTr("Off", "autocorrect")]
                descriptions: [qsTr("Also fixes real words typed by mistake"),
                               qsTr("Fixes words not in the dictionaries"),
                               qsTr("Fixes only obvious slips"),
                               ""]
            }

            SettingCombo {
                key: "learning"
                label: qsTr("Learning")
                values: ["on", "privacy", "off"]
                texts: [qsTr("On"), qsTr("Undos only"), qsTr("Off", "learning")]
                descriptions: [qsTr("Learns your words and what you type after them"),
                               qsTr("Suggests nothing learned; only stops corrections you undo"),
                               qsTr("Learns nothing")]
            }

            ValueButton {
                label: qsTr("Learned words")
                value: learnedWords.count
                onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLearned.qml"), { "learned": learnedWords })
            }

            ValueButton {
                label: qsTr("Typing aids")
                value: {
                    var on = (page._on(doubleSpacePeriodConfig.value, true) ? 1 : 0)
                            + (page._on(spaceBeforeConfig.value, false) ? 1 : 0)
                            + (page._on(replaceEmoticonsConfig.value, false) ? 1 : 0)
                    // The number of typing aids on.
                    return on > 0 ? qsTr("%n on", "typing aids", on) : qsTr("Off", "typing aids")
                }
                onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishTypingAids.qml"))
            }
        }

        VerticalScrollDecorator {}
    }
}
