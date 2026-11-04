// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// A Slider for one dconf key under /apps/parrotfish-keyboard/ holding a
// whole number (written as a string, as the layout reads it), shown with
// `suffix`; unset or out of range: `defaultValue`. Written as it moves, so
// an open keyboard follows.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Slider {
    id: slider

    property string key
    property int defaultValue
    property string suffix: "%"
    readonly property int stored: {
        var v = parseInt(config.value)
        return v >= minimumValue && v <= maximumValue ? v : defaultValue
    }

    width: parent ? parent.width : 0
    stepSize: 1
    value: stored
    valueText: Math.round(value) + suffix
    onValueChanged: if (Math.round(value) !== stored) config.value = String(Math.round(value))

    ConfigurationValue {
        id: config
        key: "/apps/parrotfish-keyboard/" + slider.key
    }
}
