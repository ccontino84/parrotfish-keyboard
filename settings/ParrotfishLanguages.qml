// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The typing languages, by script (each layout types its script's).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0
import Nemo.Configuration 1.0

Page {
    id: page

    readonly property var codes: Languages.selectedAll(languagesConfig.value)
    // The scripts of the installed languages, Latin first.
    readonly property var scripts: {
        var out = []
        var order = ["Latn", "Cyrl", "Grek"]
        for (var i = 0; i < order.length; ++i)
            for (var j = 0; j < Languages.all.length; ++j)
                if (Languages.all[j].script === order[i]) {
                    out.push(order[i])
                    break
                }
        return out
    }
    function count(script) {
        var n = 0
        for (var i = 0; i < codes.length; ++i)
            if (Languages.script(codes[i]) === script)
                ++n
        return n
    }
    function scriptName(script) {
        return script === "Cyrl" ? qsTr("Cyrillic") : script === "Grek" ? qsTr("Greek") : qsTr("Latin")
    }

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
                visible: page.count("Latn") > Languages.comfortable || page.count("Cyrl") > Languages.comfortable
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
                model: page.scripts

                Column {
                    readonly property string script: modelData
                    width: column.width

                    SectionHeader {
                        visible: page.scripts.length > 1
                        text: page.scriptName(parent.script)
                    }

                    Repeater {
                        model: Languages.all

                        TextSwitch {
                            readonly property bool on: page.codes.indexOf(modelData.code) >= 0

                            visible: modelData.script === parent.script
                            automaticCheck: false
                            checked: on
                            text: modelData.name
                            description: modelData.experimental ? qsTr("Experimental") : ""
                            // A Latin layout always has one; the others may have none.
                            enabled: !on || modelData.script !== "Latn" || page.count("Latn") > 1
                            onClicked: languagesConfig.value = Languages.toggled(page.codes, modelData.code)
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
