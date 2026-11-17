// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A dot on a key whose long press opens a menu in the current view (as
// Jolla's Popper decides), in its KeyBackground's (`frame`) corner; fainter
// when the menu has nothing for the typing languages.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: hint

    property Item frame   // the key's KeyBackground
    readonly property Item key: parent
    readonly property Item layout: frame ? frame.layout : null
    readonly property bool on: layout !== null && layout.longPressHint === true
    readonly property var _attributes: layout && layout.attributes ? layout.attributes : null
    readonly property bool _symbols: _attributes !== null && (_attributes.inSymView || _attributes.inSymView2)
    readonly property bool _always: typeof key.popupAlways === "boolean" && key.popupAlways
    readonly property string _accents: _attributes !== null && _attributes.isShifted
                                       ? key.accentsShifted : key.accents
    readonly property int _thickness: Math.max(2, Math.round(3.5 * Theme.pixelRatio))
    readonly property bool _faint: typeof _accents === "string" && _accents.length === 2 && _accents.charAt(1) === "\u22ef"
    readonly property color _color: Theme.rgba(key.palette.primaryColor, _faint ? 0.15 : 0.4)

    anchors.fill: frame
    visible: on && !key.pressed && typeof _accents === "string" && _accents.length > 0
             && (_always || !_symbols)
    z: -1   // above the background, below the label

    // In the corner, inside the background's rounding.
    Rectangle {
        width: hint._thickness
        height: width
        radius: width / 2
        color: hint._color
        anchors {
            top: parent.top
            right: parent.right
            margins: Math.round(Math.max(1.5 * hint._thickness, 0.7 * (hint.frame ? hint.frame.radius : 0)))
        }
    }
}
