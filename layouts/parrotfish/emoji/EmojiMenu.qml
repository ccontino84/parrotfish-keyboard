// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The emoji panel's long-press menu: a row each for the person, the man and
// the woman, each with its skin tones. The panel opens it, moves the active
// cell with the finger and types it; `removeCell`: a recent emoji's delete
// icon.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

Rectangle {
    id: menu

    property var rows: []        // arrays of emoji
    property real cellWidth
    property real cellHeight
    property int activeRow: -1
    property int activeColumn: -1
    readonly property int columns: {
        var n = 0
        for (var r = 0; r < rows.length; ++r)
            n = Math.max(n, rows[r].length)
        return n
    }
    readonly property string activeEmoji: activeRow >= 0 && activeRow < rows.length
                                          && activeColumn >= 0 && activeColumn < rows[activeRow].length
                                          ? rows[activeRow][activeColumn] : ""
    readonly property real padding: Theme.paddingSmall
    readonly property string removeCell: "remove"

    width: columns * cellWidth + 2 * padding
    height: rows.length * cellHeight + 2 * padding
    radius: typeof geometry !== "undefined" ? geometry.popperRadius : Theme.paddingMedium
    color: typeof keyboard !== "undefined" && keyboard.popperBackgroundColor !== undefined
           ? keyboard.popperBackgroundColor : Theme.highlightDimmerColor

    Repeater {
        model: menu.rows.length
        Item {
            id: rowItem
            readonly property int row: index
            Repeater {
                model: menu.rows[rowItem.row]
                Item {
                    x: menu.padding + index * menu.cellWidth
                    y: menu.padding + rowItem.row * menu.cellHeight
                    width: menu.cellWidth
                    height: menu.cellHeight
                    readonly property bool active: menu.activeRow === rowItem.row && menu.activeColumn === index

                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(parent.width, parent.height) * 0.95
                        height: width
                        radius: width / 4
                        color: Theme.highlightBackgroundColor
                        opacity: 0.4
                        visible: parent.active
                    }
                    Image {
                        anchors.centerIn: parent
                        readonly property int side: Math.round(Math.min(parent.width, parent.height) * 0.75)
                        width: side
                        height: side
                        sourceSize.width: side
                        sourceSize.height: side
                        asynchronous: true
                        visible: modelData !== menu.removeCell
                        source: side > 0 && visible ? EmojiUtil.source(modelData) : ""
                    }
                    Icon {
                        anchors.centerIn: parent
                        visible: modelData === menu.removeCell
                        source: visible ? "image://theme/icon-m-delete" : ""
                        highlighted: parent.active
                    }
                }
            }
        }
    }
}
