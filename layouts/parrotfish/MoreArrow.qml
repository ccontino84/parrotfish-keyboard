// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The › / ‹ cell of a long-press menu (LongPressMenu), drawn as strokes in
// the highlight colour so that it doesn't read as a character to type.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: arrow

    property int direction: 1   // 1: ›, -1: ‹
    property bool active

    // Height of the chevron; its strokes meet at the tip.
    readonly property real _size: Math.round(geometry.popperFontSize * 0.42)
    readonly property real _arm: _size / Math.SQRT2
    readonly property real _thickness: Math.max(2, Math.round(geometry.popperFontSize * 0.06))
    readonly property color _color: active ? Theme.highlightColor : Theme.secondaryHighlightColor

    width: geometry.accentPopperCellWidth
    height: geometry.popperHeight

    Item {
        // The chevron points right; mirrored for ‹.
        anchors.fill: parent
        transform: Scale { origin.x: arrow.width / 2; xScale: arrow.direction < 0 ? -1 : 1 }

        Repeater {
            model: [45, -45]
            Rectangle {
                width: arrow._arm + arrow._thickness / 2
                height: arrow._thickness
                radius: height / 2
                color: arrow._color
                antialiasing: true
                // The tip, a quarter of the chevron's height right of the middle.
                x: arrow.width / 2 + arrow._size / 4 - width + arrow._thickness / 2
                y: arrow.height / 2 - height / 2
                transformOrigin: Item.Right
                rotation: modelData
            }
        }
    }
}
