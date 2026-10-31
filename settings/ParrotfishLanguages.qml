// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The typing languages, opened from Parrotfish's section in Settings → Text
// input (ParrotfishSettings.qml). One switch per language; the last one
// can't be switched off. The keyboard watches the dconf key, so changes
// apply at once.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "languages.js" as Languages

Page {
    id: page

    readonly property var codes: Languages.selected(languagesConfig.value, Qt.locale().name)

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Typing languages")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("The keyboard follows the one you type in.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.codes.length > Languages.comfortable
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("More than %1 can make typing slower.").arg(Languages.comfortable)
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }

            Repeater {
                model: Languages.all

                TextSwitch {
                    readonly property bool on: page.codes.indexOf(modelData.code) >= 0

                    automaticCheck: false
                    checked: on
                    text: modelData.name
                    enabled: !on || page.codes.length > 1
                    onClicked: languagesConfig.value = Languages.toggled(page.codes, modelData.code)
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
