// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Parrotfish's section in Settings → Text input: the version and three
// rows, each opening a page (Typing, Layout, Look & feel) and showing its
// main choice. Jolla's text input page loads it (a Loader per settings=
// page of the enabled layouts' handlers), so it's a plain Column; Jolla
// shows it while the Parrotfish layout is enabled. Every value is a dconf
// key the keyboard watches: changes apply at once. A double tap on the
// header opens the diagnostics (ParrotfishDiagnostics.qml), for bug
// reports.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0
import "languages.js" as Languages

Column {
    id: section

    KeyboardInfo {
        id: info
    }

    ConfigurationValue {
        id: languagesConfig
        key: "/apps/parrotfish-keyboard/languages"
        defaultValue: ""
    }

    ConfigurationValue {
        id: layoutConfig
        key: "/apps/parrotfish-keyboard/layout"
        defaultValue: Languages.layoutFor(Qt.locale().name)
    }

    ConfigurationValue {
        id: fillConfig
        key: "/apps/parrotfish-keyboard/key-fill"
        defaultValue: "flat"
    }

    ConfigurationValue {
        id: borderConfig
        key: "/apps/parrotfish-keyboard/key-border"
        defaultValue: "line"
    }

    Item {
        width: parent.width
        height: header.height + version.height

        SectionHeader {
            id: header
            text: "Parrotfish"
        }

        Label {
            id: version
            anchors.top: header.bottom
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            horizontalAlignment: Text.AlignRight
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: qsTr("Version %1").arg(info.version)
        }

        // Hidden, for support.
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishDiagnostics.qml"), { "info": info })
        }
    }

    ValueButton {
        label: qsTr("Typing")
        value: Languages.names(Languages.selected(languagesConfig.value, Qt.locale().name))
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishTyping.qml"))
    }

    ValueButton {
        readonly property var names: ({ "qwerty": "QWERTY", "qwertz": "QWERTZ", "azerty": "AZERTY",
                                        "nordic": "QWERTY (ÅÖÄ)" })
        label: qsTr("Layout")
        value: names[layoutConfig.value] || names[Languages.layoutFor(Qt.locale().name)]
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLayout.qml"))
    }

    ValueButton {
        readonly property var fills: ({ "none": qsTr("No fill"), "flat": qsTr("Flat"), "shaded": qsTr("Shaded") })
        readonly property var borders: ({ "none": qsTr("no border"), "line": qsTr("line"), "raised": qsTr("raised") })
        label: qsTr("Look & feel")
        value: (fills[fillConfig.value] || fills["flat"]) + ", " + (borders[borderConfig.value] || borders["line"])
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLook.qml"))
    }
}
