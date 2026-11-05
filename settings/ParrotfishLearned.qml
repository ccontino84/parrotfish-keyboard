// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The learned words, opened from Parrotfish's section in Settings → Text
// input (ParrotfishSettings.qml): the words learned as new and those whose
// correction was undone, one row each, under the name of their language.
// Forget one in its language (long press); clear everything learned in one
// language (pull-down menu, then the language) or in all, each with
// Sailfish's remorse timer.
// The keyboard is closed while Settings is open: this page edits
// learning.db itself and bumps a dconf key for the keyboard to reload.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page

    property QtObject learned   // a LearnedWords

    function tellKeyboard() {
        changedConfig.value = String(Date.now())
    }

    ConfigurationValue {
        id: changedConfig
        key: "/apps/parrotfish-keyboard/learning-changed"
        defaultValue: ""
    }

    RemorsePopup {
        id: clearRemorse
    }

    // The languages with learned data (also those without listed words).
    Component {
        id: languagePage

        Page {
            id: chooser

            property var languages: page.learned.languages()

            SilicaListView {
                anchors.fill: parent
                model: chooser.languages

                header: PageHeader {
                    title: qsTr("Clear one language")
                }

                ViewPlaceholder {
                    enabled: chooser.languages.length === 0
                    text: qsTr("Nothing learned yet")
                }

                delegate: BackgroundItem {
                    id: languageItem

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        anchors.verticalCenter: parent.verticalCenter
                        truncationMode: TruncationMode.Fade
                        text: modelData.name
                        color: languageItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }

                    onClicked: {
                        var code = modelData.code
                        pageStack.pop()
                        clearRemorse.execute(qsTr("Clearing %1").arg(modelData.name), function() {
                            page.learned.clearLanguage(code)
                            page.tellKeyboard()
                        })
                    }
                }

                VerticalScrollDecorator {}
            }
        }
    }

    SilicaListView {
        id: list

        anchors.fill: parent
        model: page.learned

        header: PageHeader {
            title: qsTr("Learned words")
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Clear one language…")
                onClicked: pageStack.push(languagePage)
            }
            MenuItem {
                text: qsTr("Clear all learned data")
                onClicked: clearRemorse.execute(qsTr("Clearing learned data"), function() {
                    page.learned.clearAll()
                    page.tellKeyboard()
                })
            }
        }

        ViewPlaceholder {
            enabled: page.learned.count === 0
            text: qsTr("No learned words")
            hintText: qsTr("Words you keep typing appear here")
        }

        section.property: "languageName"
        section.delegate: SectionHeader {
            text: section
        }

        delegate: ListItem {
            id: item

            function forget() {
                var word = model.word
                var language = model.language
                remorseDelete(function() {
                    page.learned.forget(word, language)
                    page.tellKeyboard()
                })
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Forget")
                    onClicked: item.forget()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                truncationMode: TruncationMode.Fade
                text: model.word
                color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
