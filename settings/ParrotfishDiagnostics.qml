// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// What a bug report needs, to copy: the version, the system, the settings
// (dconf, "-" if unset: the default) and what was learned, as counts per
// language. No word the user typed. Opened by a double tap on Parrotfish's
// section header (ParrotfishSettings.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property QtObject info
    property string system: ""
    // The keys the settings write (and the keyboard reads).
    readonly property var keys: ["languages", "layout", "autocorrect", "learning", "number-row",
        "number-row-height", "sticky", "sticky-side", "swipe", "key-height", "key-height-landscape",
        "key-gap", "key-fill", "key-border", "long-press-hint", "dividers", "space-before-punctuation"]
    property var values: ({})
    readonly property string text: {
        var lines = ["Parrotfish " + (info ? info.version : "?"),
                     "system " + (system !== "" ? system : "unknown"),
                     "debug " + (info && info.debug ? "on" : "off"), ""]
        for (var i = 0; i < keys.length; ++i)
            lines.push(keys[i] + " " + (values[keys[i]] !== undefined ? values[keys[i]] : "-"))
        lines.push("")
        var learned = info ? info.learningSummary() : []
        for (i = 0; i < learned.length; ++i)
            lines.push(learned[i])
        return lines.join("\n")
    }

    Component.onCompleted: {
        var read = {}
        for (var i = 0; i < keys.length; ++i) {
            var v = Qt.createQmlObject("import Nemo.Configuration 1.0; ConfigurationValue { key: \"/apps/parrotfish-keyboard/"
                                       + keys[i] + "\" }", page)
            if (v.value !== undefined && v.value !== null && v.value !== "")
                read[keys[i]] = String(v.value)
            v.destroy()
        }
        values = read
        var request = new XMLHttpRequest()
        request.onreadystatechange = function() {
            if (request.readyState === XMLHttpRequest.DONE) {
                var m = /^PRETTY_NAME="?([^"\n]*)/m.exec(request.responseText || "")
                page.system = m ? m[1] : ""
            }
        }
        request.open("GET", "file:///etc/os-release")
        request.send()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Diagnostics")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                font.family: "monospace"
                color: Theme.highlightColor
                text: page.text
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Copy")
                onClicked: Clipboard.text = page.text
            }
        }

        VerticalScrollDecorator {}
    }
}
