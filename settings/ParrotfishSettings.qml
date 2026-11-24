// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Parrotfish's section in Settings → Text input. Jolla's page loads it (a
// Loader per settings= of the enabled layouts' handlers), so it's a plain
// Column. Every value is a dconf key the keyboard watches.
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
        id: enabledLayoutsConfig
        key: "/sailfish/text_input/enabled_layouts"
        defaultValue: []
    }

    ConfigurationValue {
        id: numberRowConfig
        key: "/apps/parrotfish-keyboard/number-row"
        defaultValue: true
    }

    ConfigurationValue {
        id: numpadConfig
        key: "/apps/parrotfish-keyboard/numpad"
        defaultValue: false
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
        id: voiceConfig
        key: "/apps/parrotfish-keyboard/voice"
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

        MouseArea {
            anchors.fill: parent   // the title and the version: a larger target
            onDoubleClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishDiagnostics.qml"), { "info": info })
        }
    }

    ValueButton {
        label: qsTr("Typing")
        value: {
            var revision = Languages.revision
            var codes = Languages.typingAll(languagesConfig.value, enabledLayoutsConfig.value)
            if (codes.length === 0)
                return qsTr("No languages")
            return Languages.names(codes)
        }
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishTyping.qml"))
    }

    // The keys that are on, as a list: "Number row, sticky key".
    ValueButton {
        label: qsTr("Layout")
        value: {
            var on = []
            if (String(numpadConfig.value) === "true")
                on.push(qsTr("numpad", "in a list"))
            else if (numberRowConfig.value !== false && numberRowConfig.value !== "false")
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
        label: qsTr("Appearance")
        value: (fills[fillConfig.value] || fills["none"]) + ", " + (borders[borderConfig.value] || borders["none"])
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishAppearance.qml"))
    }

    ValueButton {
        visible: VoiceModel.installed
        label: qsTr("Voice input")
        value: VoiceModel.state === "downloading" ? qsTr("Downloading the model")
               : !VoiceModel.ready ? qsTr("Model not downloaded")
               : voiceConfig.value !== false && voiceConfig.value !== "false" ? qsTr("On", "voice input") : qsTr("Off", "voice input")
        onClicked: pageStack.animatorPush(Qt.resolvedUrl("ParrotfishVoice.qml"))
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
