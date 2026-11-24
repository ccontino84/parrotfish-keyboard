// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The typing languages: the ones on the phone (from a language package or
// downloaded) switched on or off, their updates, and the ones to download
// (LanguageDownloads, rows in LanguageGetItem.qml).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0
import Nemo.Configuration 1.0

Page {
    id: page

    // The languages typed in, as the keyboard picks them: none of an
    // alphabet whose Parrotfish layouts are all off in Jolla's Keyboards.
    readonly property var typed: {
        var revision = Languages.revision
        return Languages.typingAll(languagesConfig.value, enabledLayoutsConfig.value)
    }
    readonly property var scripts: ["Latn", "Cyrl", "Grek"]
    readonly property var scriptsOn: Languages.scriptsOn(enabledLayoutsConfig.value)
    // A Parrotfish layout is on (else enabled_layouts is unread: all count as on).
    readonly property bool layoutsKnown: {
        var layouts = enabledLayoutsConfig.value || []
        for (var i = 0; i < layouts.length; ++i)
            if (String(layouts[i]).indexOf("parrotfish") === 0)
                return true
        return false
    }
    readonly property var layoutNames: ({ "Latn": "Auto (QWERTY)", "Cyrl": "Auto (ЙЦУКЕН)", "Grek": "Auto (Ελληνικά)" })
    function count(script) {
        var n = 0
        for (var i = 0; i < typed.length; ++i)
            n += Languages.script(typed[i]) === script ? 1 : 0
        return n
    }
    function scriptName(script) {
        return script === "Cyrl" ? qsTr("Cyrillic") : script === "Grek" ? qsTr("Greek") : qsTr("Latin")
    }
    // The dconf value's languages of the alphabets with no layout on: kept
    // for when one is turned on again.
    function keptOff() {
        var parts = String(languagesConfig.value).split(" ")
        return parts.filter(function(code) {
            return code !== "" && scriptsOn.indexOf(Languages.script(code)) < 0
        })
    }
    function countIn(script, key) {
        var n = 0
        for (var i = 0; i < Languages.all.length; ++i)
            n += Languages.all[i].script === script && Languages.all[i][key] ? 1 : 0
        return n
    }
    function downloadable(l) {
        return !l.installed && l.size > 0
    }
    // One row per language in each section, shown or hidden: rows rebuilt
    // under an open menu or a remorse break the page.
    readonly property int languageCount: Languages.all.length
    readonly property int downloadableCount: {
        var n = 0
        for (var i = 0; i < Languages.all.length; ++i)
            n += downloadable(Languages.all[i]) ? 1 : 0
        return n
    }
    readonly property bool anyPackaged: _count("packaged", true) > 0
    readonly property int updateCount: _count("update", true)
    readonly property real updateBytes: {
        var n = 0
        for (var i = 0; i < Languages.all.length; ++i)
            n += Languages.all[i].update ? Languages.all[i].size : 0
        return n
    }
    // Indexes into Languages.all: the system language first.
    readonly property var order: {
        var first = [], rest = []
        for (var i = 0; i < Languages.all.length; ++i)
            (Languages.all[i].code === Languages.systemLanguage ? first : rest).push(i)
        return first.concat(rest)
    }
    readonly property bool mobileData: mobileDataLoader.item ? mobileDataLoader.item.mobileData : false
    // Tapped once on mobile data: the next tap downloads it.
    property string armed: ""
    // The last language on in its alphabet, tapped: told why it stays on.
    property string keptOn: ""

    function _count(key, value) {
        var n = 0
        for (var i = 0; i < Languages.all.length; ++i)
            n += Languages.all[i][key] === value ? 1 : 0
        return n
    }

    function megabytes(bytes) {
        return Math.max(1, Math.round(bytes / 1000000))
    }

    function errorText(error) {
        var parts = error.split(" ")
        var kind = parts[0]
        if (kind === "space")
            return qsTr("Not enough storage: %1 MB more needed").arg(megabytes(Number(parts[1])))
        if (kind === "network")
            return qsTr("The download failed")
        if (kind === "checksum")
            return qsTr("A downloaded file was damaged")
        if (kind === "write")
            return qsTr("The language couldn't be saved")
        if (kind === "unavailable")
            return qsTr("Not available for download")
        return qsTr("The download stopped")
    }

    // A download's state as a description, or "" if none.
    function jobText(code) {
        var job = LanguageDownloads.jobs[code]
        if (!job)
            return ""
        if (job.state === "queued")
            return qsTr("Waiting")
        if (job.state === "verifying")
            return qsTr("Checking")
        if (job.state === "failed")
            return errorText(job.error)
        return qsTr("%1 of %2 MB").arg((job.received / 1000000).toFixed(1)).arg(megabytes(job.total))
    }

    function busy(code) {
        var job = LanguageDownloads.jobs[code]
        return !!job && job.state !== "failed"
    }

    function source(l) {
        // The copy in use.
        var text = l.source === "download" ? qsTr("Downloaded") : qsTr("Package")
        if (l.experimental)
            text = qsTr("Experimental") + " · " + text
        return text
    }

    function getLanguage(code) {
        if (mobileData && armed !== code) {
            armed = code
            return
        }
        armed = ""
        LanguageDownloads.download(code)
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

    Loader {
        id: mobileDataLoader
        source: "ParrotfishMobileData.qml"
    }

    Component.onCompleted: Languages.refresh()

    Timer {
        id: keptOnTimer
        interval: 3000
        onTriggered: page.keptOn = ""
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            visible: page.updateCount > 0
            MenuItem {
                // On mobile data the item says so: choosing it is the confirmation.
                text: page.mobileData ? qsTr("Update all on mobile data (%1 MB)").arg(page.megabytes(page.updateBytes))
                                      : qsTr("Update all (%1 MB)").arg(page.megabytes(page.updateBytes))
                onClicked: LanguageDownloads.updateAll()
            }
        }

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
                text: qsTr("Latin keyboards follow the language you type in.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.count("Latn") > Languages.comfortable
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("More than %1 can make typing slower.").arg(Languages.comfortable)
            }

            // An alphabet's languages on the phone, switched on or off.
            Repeater {
                model: page.scripts

                Column {
                    id: scriptSection

                    readonly property string script: modelData
                    readonly property bool layoutOn: page.scriptsOn.indexOf(script) >= 0
                    readonly property int installed: page.countIn(script, "installed")

                    width: parent.width
                    visible: installed > 0 || (page.layoutsKnown && layoutOn)

                    SectionHeader {
                        text: page.scriptName(scriptSection.script)
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        visible: text !== ""
                        wrapMode: Text.Wrap
                        color: Theme.secondaryHighlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        text: scriptSection.installed === 0 ? qsTr("No languages yet: download one below.")
                              : !scriptSection.layoutOn ? qsTr("Turn on %1 in Keyboards to type these.").arg(page.layoutNames[scriptSection.script])
                              : ""
                    }

                    Repeater {
                        model: page.languageCount

                        ListItem {
                            id: installedItem

                            readonly property var language: Languages.all[index]
                            readonly property bool on: page.typed.indexOf(language.code) >= 0
                            // Each alphabet keeps one language on.
                            readonly property bool canSwitch: !on || page.count(language.script) > 1

                            function toggle() {
                                if (!scriptSection.layoutOn)
                                    return
                                if (canSwitch) {
                                    languagesConfig.value = Languages.toggled(page.typed.concat(page.keptOff()), language.code)
                                } else {
                                    page.keptOn = language.code
                                    keptOnTimer.restart()
                                }
                            }

                            visible: language.installed && language.script === scriptSection.script
                            contentHeight: Theme.itemSizeMedium
                            onClicked: toggle()
                            menu: ContextMenu {
                                MenuLabel {
                                    visible: !installedItem.language.downloaded
                                    text: qsTr("Installed with a language package")
                                }
                                MenuItem {
                                    visible: installedItem.language.downloaded
                                    text: qsTr("Delete")
                                    // The page's remorse: the rows are rebuilt when it's done.
                                    onClicked: {
                                        var code = installedItem.language.code
                                        Remorse.popupAction(page, installedItem.language.packaged
                                                            ? qsTr("Back to the package's copy")
                                                            : qsTr("Deleted %1").arg(installedItem.language.name),
                                                            function() { LanguageDownloads.remove(code) })
                                    }
                                }
                            }

                            Switch {
                                id: languageSwitch
                                x: Theme.horizontalPageMargin - Theme.paddingLarge
                                anchors.verticalCenter: parent.verticalCenter
                                automaticCheck: false
                                checked: installedItem.on
                                opacity: scriptSection.layoutOn ? 1 : 0.4
                                onClicked: installedItem.toggle()
                            }

                            Column {
                                anchors {
                                    left: languageSwitch.right
                                    right: parent.right
                                    rightMargin: Theme.horizontalPageMargin
                                    verticalCenter: parent.verticalCenter
                                }
                                opacity: scriptSection.layoutOn ? 1 : 0.4

                                Label {
                                    width: parent.width
                                    truncationMode: TruncationMode.Fade
                                    text: installedItem.language.name
                                    color: installedItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                                }
                                Label {
                                    width: parent.width
                                    truncationMode: TruncationMode.Fade
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    color: installedItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                                    text: page.keptOn === installedItem.language.code ? qsTr("The keyboard needs one language")
                                          : page.source(installedItem.language)
                                }
                            }
                        }
                    }
                }
            }

            SectionHeader {
                visible: page.updateCount > 0
                text: qsTr("Updates")
            }

            Repeater {
                model: page.languageCount

                LanguageGetItem {
                    languagesPage: page
                    language: Languages.all[index]
                    visible: language.update
                }
            }

            SectionHeader {
                visible: page.downloadableCount > 0
                text: qsTr("Download")
            }

            // By alphabet, each with the system language first.
            Repeater {
                model: page.scripts

                Column {
                    id: downloadGroup

                    readonly property string script: modelData
                    readonly property int count: {
                        var n = 0
                        for (var i = 0; i < Languages.all.length; ++i)
                            n += Languages.all[i].script === script && page.downloadable(Languages.all[i]) ? 1 : 0
                        return n
                    }

                    width: parent.width
                    visible: count > 0

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        height: implicitHeight + Theme.paddingMedium
                        verticalAlignment: Text.AlignBottom
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        text: page.scriptName(downloadGroup.script)
                    }

                    Repeater {
                        model: page.languageCount

                        LanguageGetItem {
                            languagesPage: page
                            language: Languages.all[page.order[index]]
                            visible: language.script === downloadGroup.script && page.downloadable(language)
                        }
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
                visible: page.anyPackaged
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.anyPackaged
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("Languages from a package are removed by uninstalling the package, together with its other languages.")
            }
        }

        VerticalScrollDecorator {}
    }
}
