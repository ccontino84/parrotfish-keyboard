// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Voice input: the switch and the speech model's download (VoiceModel).
import QtQuick 2.0
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

Page {
    id: page

    readonly property real modelSize: 670576835
    readonly property bool mobileData: mobileDataLoader.item ? mobileDataLoader.item.mobileData : false

    function megabytes(bytes) {
        return Math.round(bytes / 1000000)
    }

    function errorText(error) {
        var parts = error.split(" ")
        var kind = parts[0]
        if (kind === "space")
            return qsTr("Not enough storage: %1 MB more needed").arg(megabytes(Number(parts[1])))
        if (kind === "network")
            return qsTr("The download failed: %1").arg(parts.slice(1).join(" "))
        if (kind === "checksum")
            return qsTr("A downloaded file was damaged")
        if (kind === "write")
            return qsTr("The model couldn't be saved")
        return qsTr("The download stopped")
    }

    Loader {
        id: mobileDataLoader
        source: "ParrotfishMobileData.qml"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Voice input")
            }

            SettingSwitch {
                key: "voice"
                defaultValue: true
                enabled: VoiceModel.ready
                text: qsTr("Voice input")
                description: qsTr("Hold the spacebar and lift the finger on the microphone. Speech is recognised on the phone")
            }

            SectionHeader {
                text: qsTr("Speech model")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: VoiceModel.ready
                      ? qsTr("Downloaded: %1 MB").arg(page.megabytes(VoiceModel.diskUsage))
                      : qsTr("Voice input needs a speech model, downloaded once from Hugging Face (%1 MB). Nothing else uses the network.")
                        .arg(page.megabytes(page.modelSize))
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            // Not downloaded.
            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: !VoiceModel.ready && VoiceModel.state !== "downloading" && VoiceModel.state !== "verifying"

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    visible: VoiceModel.state === "failed"
                    color: Theme.errorColor !== undefined ? Theme.errorColor : Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: page.errorText(VoiceModel.error) + ". " + qsTr("Try again: it continues where it stopped.")
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    visible: page.mobileData
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: qsTr("You're on mobile data. The download is %1 MB: Wi-Fi is better.")
                          .arg(page.megabytes(page.modelSize - VoiceModel.diskUsage))
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: page.mobileData ? qsTr("Download anyway")
                          : VoiceModel.state === "failed" || VoiceModel.diskUsage > 0 ? qsTr("Continue download")
                          : qsTr("Download")
                    onClicked: VoiceModel.download()
                }
            }

            // Downloading or checking.
            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: VoiceModel.state === "downloading" || VoiceModel.state === "verifying"

                ProgressBar {
                    width: parent.width
                    indeterminate: VoiceModel.state === "verifying" || VoiceModel.total <= 0
                    minimumValue: 0
                    maximumValue: Math.max(1, VoiceModel.total)
                    value: VoiceModel.received
                    label: VoiceModel.state === "verifying" ? qsTr("Checking the files")
                           : qsTr("%1 of %2 MB").arg(page.megabytes(VoiceModel.received))
                                                .arg(page.megabytes(VoiceModel.total || page.modelSize))
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: VoiceModel.state === "downloading"
                    text: qsTr("Stop")
                    onClicked: VoiceModel.cancel()
                }
            }

            // Downloaded.
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: VoiceModel.ready
                text: qsTr("Delete model")
                onClicked: Remorse.popupAction(page, qsTr("Speech model deleted"), function() { VoiceModel.remove() })
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("Model: NVIDIA Parakeet TDT 0.6B v3, post-trained by Moondream (parakeet-ultra), converted by the sherpa-onnx project. Licence: CC BY 4.0.")
            }
        }

        VerticalScrollDecorator {}
    }
}
