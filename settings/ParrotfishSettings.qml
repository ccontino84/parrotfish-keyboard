// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Parrotfish's section in Settings → Text input: the version and three
// rows, each opening a page (Typing, Layout, Look & feel) and showing its
// main choices. Jolla's text input page loads it (a Loader per settings=
// page of the enabled layouts' handlers), so it's a plain Column; Jolla
// shows it while the Parrotfish layout is enabled. Every value is a dconf
// key the keyboard watches: changes apply at once. Below the rows, Reset
// (every dconf key unset, as on a new installation). A double tap on the
// version opens the diagnostics, with Report a bug (ParrotfishDiagnostics.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Parrotfish.Keyboard 1.0

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
        id: numberRowConfig
        key: "/apps/parrotfish-keyboard/number-row"
        defaultValue: true
    }

    ConfigurationValue {
        id: languageKeysConfig
        key: "/apps/parrotfish-keyboard/language-keys"
        defaultValue: false
    }

    ConfigurationValue {
        id: stickyConfig
        key: "/apps/parrotfish-keyboard/sticky"
        defaultValue: true
    }

    ConfigurationValue {
        id: fillConfig
        key: "/apps/parrotfish-keyboard/key-fill"
        defaultValue: "none"
    }

    ConfigurationValue {
        id: borderConfig
        key: "/apps/parrotfish-keyboard/key-border"
        defaultValue: "none"
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

        // For support: the diagnostics and Report a bug.
        MouseArea {
            anchors.fill: parent   // the title and the version: a larger target
            onDoubleClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishDiagnostics.qml"), { "info": info })
        }
    }

    ValueButton {
        label: qsTr("Typing")
        value: Languages.names(Languages.selected(languagesConfig.value))
                       || qsTr("No languages installed")
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishTyping.qml"))
    }

    // The keys that are on, as a list: "Number row, sticky key".
    ValueButton {
        label: qsTr("Layout")
        value: {
            var on = []
            if (numberRowConfig.value !== false && numberRowConfig.value !== "false")
                on.push(qsTr("number row", "in a list"))
            if (String(languageKeysConfig.value) === "true")
                on.push(qsTr("language keys", "in a list"))
            if (stickyConfig.value !== false && stickyConfig.value !== "false")
                on.push(qsTr("sticky key", "in a list"))
            var text = on.join(", ") || qsTr("Off", "layout")
            return text.charAt(0).toUpperCase() + text.slice(1)
        }
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLayout.qml"))
    }

    ValueButton {
        readonly property var fills: ({ "none": qsTr("No fill"), "flat": qsTr("Flat"), "shaded": qsTr("Shaded") })
        readonly property var borders: ({ "none": qsTr("no border"), "line": qsTr("line"), "raised": qsTr("raised") })
        label: qsTr("Look & feel")
        value: (fills[fillConfig.value] || fills["none"]) + ", " + (borders[borderConfig.value] || borders["none"])
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishLook.qml"))
    }

    // The page Jolla loaded this section into (for the remorse popup).
    function _page() {
        for (var item = section.parent; item; item = item.parent)
            if (item.status !== undefined && item.backNavigation !== undefined)
                return item
        return section
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
    }

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Reset")
        onClicked: Remorse.popupAction(section._page(), qsTr("Settings back to defaults"),
                                       function() { info.resetSettings() })
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
    }
}
