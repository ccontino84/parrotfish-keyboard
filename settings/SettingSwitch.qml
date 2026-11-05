// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A TextSwitch for one boolean dconf key under /apps/parrotfish-keyboard/
// (true/false, also as strings); unset: `defaultValue`.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

TextSwitch {
    id: toggle

    property string key
    property bool defaultValue: false
    readonly property bool on: config.value === undefined || config.value === null || config.value === ""
                               ? defaultValue : config.value === true || config.value === "true"

    automaticCheck: false
    checked: on
    onClicked: config.value = !on

    ConfigurationValue {
        id: config
        key: "/apps/parrotfish-keyboard/" + toggle.key
    }
}
