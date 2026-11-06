// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Typing: languages, autocorrect, learning. Opened from Parrotfish's
// section in Settings → Text input (ParrotfishSettings.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0

Page {
    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

    LearnedWords {
        id: learnedWords
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

            SettingSwitch {
                key: "space-before-punctuation"
                text: qsTr("Space before ! ? : ;")
                description: qsTr("Keeps the space after a picked suggestion, as in French")
            }
        }

        VerticalScrollDecorator {}
    }
}
