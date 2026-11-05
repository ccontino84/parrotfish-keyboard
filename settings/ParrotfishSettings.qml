// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Parrotfish's section in Settings → Text input: the version and two
// rows, each opening a page (Typing, Look & feel) and showing its main
// choice. Jolla's text input page loads it (a Loader per settings=
// page of the enabled layouts' handlers), so it's a plain Column; Jolla
// shows it while the Parrotfish layout is enabled. Every value is a dconf
// key the keyboard watches: changes apply at once. Below the rows, Reset
// (every dconf key unset, as on a new installation) and Report
// a bug (an issue in the browser with the diagnostics filled in). A double
// tap on the header opens the diagnostics (ParrotfishDiagnostics.qml).
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
        value: Languages.names(Languages.selected(languagesConfig.value))
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishTyping.qml"))
    }

    ValueButton {
        readonly property var fills: ({ "none": qsTr("No fill"), "flat": qsTr("Flat"), "shaded": qsTr("Shaded") })
        readonly property var borders: ({ "none": qsTr("no border"), "line": qsTr("line"), "raised": qsTr("raised") })
        label: qsTr("Look & feel")
        value: (fills[fillConfig.value] || fills["flat"]) + ", " + (borders[borderConfig.value] || borders["line"])
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

    Row {
        id: buttons

        readonly property real buttonWidth: Math.min(Theme.buttonWidthMedium,
                                                     (section.width - 2 * Theme.horizontalPageMargin - spacing) / 2)

        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.paddingLarge

        Button {
            width: buttons.buttonWidth
            text: qsTr("Reset")
            onClicked: Remorse.popupAction(section._page(), qsTr("Settings back to defaults"),
                                           function() { info.resetSettings() })
        }

        Button {
            width: buttons.buttonWidth
            text: qsTr("Report a bug")
            // A new issue in the browser, from the bug report form
            // (.github/ISSUE_TEMPLATE/bug_report.yml), the diagnostics filled in.
            onClicked: Qt.openUrlExternally("https://github.com/ccontino84/parrotfish-keyboard/issues/new"
                                            + "?template=bug_report.yml&diagnostics="
                                            + encodeURIComponent(info.diagnostics()))
        }
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
    }
}
