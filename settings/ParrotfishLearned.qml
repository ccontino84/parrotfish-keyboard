// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The learned words, opened from Parrotfish's section in Settings → Text
// input (ParrotfishSettings.qml): the words learned as new and those whose
// correction was undone, one row each. Forget one (long press), or clear
// everything learned (pull-down menu), each with Sailfish's remorse timer.
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

    SilicaListView {
        id: list

        anchors.fill: parent
        model: page.learned

        header: PageHeader {
            title: qsTr("Learned words")
        }

        PullDownMenu {
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

        delegate: ListItem {
            id: item

            function forget() {
                var word = model.word
                remorseDelete(function() {
                    page.learned.forget(word)
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
