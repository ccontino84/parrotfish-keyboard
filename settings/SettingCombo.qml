// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// A ComboBox for one dconf key under /apps/parrotfish-keyboard/: `values`
// are written, `texts` shown, `descriptions` (optional) under the chosen
// one. An unset key or an unknown value shows `defaultValue`.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

ComboBox {
    id: combo

    property string key
    property var values: []
    property var texts: []
    property var descriptions: null
    property string defaultValue: values.length > 0 ? values[0] : ""
    readonly property string current: values.indexOf(String(config.value)) >= 0 ? String(config.value) : defaultValue

    width: parent ? parent.width : 0
    currentIndex: Math.max(0, values.indexOf(current))
    description: descriptions ? descriptions[currentIndex] : ""

    menu: ContextMenu {
        Repeater {
            model: combo.texts
            MenuItem {
                text: modelData
                onClicked: config.value = combo.values[index]
            }
        }
    }

    ConfigurationValue {
        id: config
        key: "/apps/parrotfish-keyboard/" + combo.key
    }
}
