// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A language to download or update, on ParrotfishLanguages.qml: its size
// or its download's state, and the download icon (✕ while it runs).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

ListItem {
    id: item

    // An entry of Languages.all, and the page (its download helpers).
    property var language
    property Item languagesPage
    readonly property bool busy: languagesPage.busy(language.code)

    contentHeight: Theme.itemSizeMedium
    onClicked: if (!busy) languagesPage.getLanguage(language.code)

    Column {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: actionButton.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        Label {
            width: parent.width
            truncationMode: TruncationMode.Fade
            text: item.language.name
            color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            text: {
                var l = item.language
                var job = LanguageDownloads.jobs[l.code]
                if (job && job.state === "failed")
                    return item.languagesPage.jobText(l.code) + ". " + qsTr("Tap to try again")
                if (job)
                    return item.languagesPage.jobText(l.code)
                if (item.languagesPage.armed === l.code)
                    return qsTr("Mobile data: tap again to download (%1 MB)").arg(item.languagesPage.megabytes(l.size))
                var size = qsTr("%1 MB").arg(item.languagesPage.megabytes(l.size))
                return l.experimental ? qsTr("Experimental") + " · " + size : size
            }
        }
    }

    IconButton {
        id: actionButton
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        icon.source: item.busy ? "image://theme/icon-m-clear" : "image://theme/icon-m-cloud-download"
        onClicked: item.busy ? LanguageDownloads.cancel(item.language.code) : item.languagesPage.getLanguage(item.language.code)
    }
}
