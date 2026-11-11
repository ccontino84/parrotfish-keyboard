// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The emoji panel, in place of a layout's rows while its emojiView is on.
// It knows nothing of the layout: the layout passes the recent emoji, the
// picked variants and its settings in, and stores what the signals report.
//
// Only the visible columns (and one more) have keys, reused as the grid
// scrolls (each slot keeps its column modulo their number). Jolla's
// keyboard finds the keys under a touch like any key (keyType). The
// touches are read as in SpaceCursor.qml, listening to Jolla's touch area
// without taking them; once a slide starts, the touch is cancelled in
// Jolla's keyboard (keyboard.cancelTouchPoint), so that no emoji is typed,
// and keyboard.interactive is turned off, so that it doesn't swipe to
// another layout (Jolla's keyboard turns it back on when all fingers lift).
// A menu pick is typed as Jolla's Popper types: a PopupKey given to the
// input handler.
import QtQuick 2.6
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../../.."
import "emojidata.js" as EmojiData
import Parrotfish.Keyboard 1.0

Item {
    id: panel

    property int keyHeight
    property var recent: []   // the recent emoji, most used first
    property var picked: ({})   // the variant last picked per emoji: { emoji: variant }
    property bool longPressHint: true   // dots on emoji with variants
    property bool keyboardKey: false   // a key back to the letters (split landscape: no bar)
    property bool current: true   // its layout is the one in use: it takes touches

    signal variantPicked(string emoji, string variant)
    signal recentRemoved(string emoji)
    signal closeRequested()

    // The groups: { icon, emojis (array), variants (bool) }.
    readonly property var groups: {
        var out = [{ "icon": "🕘", "emojis": recent, "variants": false }]
        for (var i = 0; i < EmojiData.groups.length; ++i) {
            var emojis = EmojiData.groups[i].emojis.split(" ")
            out.push({ "icon": EmojiData.groups[i].icon, "emojis": emojis, "variants": true })
        }
        return out
    }

    // Laid out only while shown, and in one step (_takeSize): a binding
    // seeing a new width with an old key height would make and drop
    // thousands of keys.
    property var _grid: ({ "width": 0, "height": 0, "columns": 6, "rows": 2 })
    function _takeSize() {
        var gridHeight = height - keyHeight
        if (!visible || width <= 0 || keyHeight <= 0 || gridHeight <= 0
                || (_grid.width === width && _grid.height === gridHeight))
            return
        var cell = keyHeight * 0.8
        _grid = { "width": width, "height": gridHeight,
                  "columns": Math.max(6, Math.round(width / cell)),
                  "rows": Math.max(2, Math.floor(gridHeight / cell)) }
    }
    // A rotation changes the sizes in several steps: laid out once, after.
    onWidthChanged: resize.restart()
    onHeightChanged: resize.restart()
    onKeyHeightChanged: resize.restart()
    Timer {
        id: resize
        interval: 0
        onTriggered: panel._takeSize()
    }

    readonly property real _width: _grid.width
    readonly property real gridHeight: _grid.height
    readonly property int columns: _grid.columns
    readonly property int rows: _grid.rows
    readonly property real cellWidth: _grid.width / _grid.columns
    readonly property real cellHeight: _grid.height / _grid.rows

    // Each group's first column and its number of columns.
    readonly property var groupStart: {
        var out = []
        var col = 0
        for (var g = 0; g < groups.length; ++g) {
            out.push(col)
            // The recent emoji fill the first screen (_recentOffset).
            col += Math.ceil(groups[g].emojis.length / rows) + (g === 0 ? _recentOffset : 0)
        }
        out.push(col)   // the end
        return out
    }
    readonly property int _recentOffset: recent.length > 0
                                         ? Math.max(0, columns - Math.ceil(recent.length / rows)) : 0
    readonly property int totalColumns: groupStart[groups.length]

    // The space between groups, and each group's x (the end's too): its
    // columns plus a gap before every group that has a line before it
    // (separated(), not the first nor an empty one).
    readonly property real groupGap: Math.round(cellWidth / 2)
    function separated(g) {
        return g > 0 && g < groups.length && groupStart[g] > 0 && groupStart[g] < groupStart[g + 1]
    }
    readonly property var groupX: {
        var out = []
        var gaps = 0
        for (var g = 0; g <= groups.length; ++g) {
            if (separated(g))
                gaps++
            out.push(groupStart[g] * cellWidth + gaps * groupGap)
        }
        return out
    }
    function columnX(col) {
        var g = groupAt(col)
        return groupX[g] + (col - groupStart[g]) * cellWidth
    }
    // The column at x (in a gap: the next group's first).
    function columnAt(x) {
        if (cellWidth <= 0)
            return 0
        for (var g = groups.length - 1; g > 0; --g) {
            if (separated(g) && x >= groupX[g] - groupGap)
                return groupStart[g] + Math.max(0, Math.floor((x - groupX[g]) / cellWidth))
        }
        return Math.max(0, Math.floor(x / cellWidth))
    }
    readonly property real maxScroll: Math.max(0, groupX[groups.length] - _width)

    // How far the grid is scrolled, in pixels.
    property real scrollX: 0
    readonly property int firstColumn: columnAt(scrollX)
    // The group at the left edge (most of its first visible column).
    readonly property int currentGroup: cellWidth > 0 ? groupAt(columnAt(scrollX + cellWidth / 2)) : 0

    function groupAt(col) {
        for (var g = groups.length - 1; g > 0; --g) {
            if (col >= groupStart[g] && groupStart[g] < groupStart[g + 1])
                return g
        }
        return 0
    }

    function emojiAt(col, row) {
        if (col < 0 || col >= totalColumns)
            return ""
        var g = groupAt(col)
        var i = (col - groupStart[g] - (g === 0 ? _recentOffset : 0)) * rows + row
        if (i < 0)
            return ""
        var emojis = groups[g].emojis
        return i < emojis.length ? emojis[i] : ""
    }

    function scrollTo(x, duration) {
        scroller.stop()
        scroller.to = Math.max(0, Math.min(maxScroll, x))
        scroller.duration = duration
        scroller.easing.type = Easing.OutQuad
        scroller.start()
    }

    onMaxScrollChanged: if (scrollX > maxScroll) scrollX = maxScroll
    onVisibleChanged: {
        _takeSize()
        emojiMenu.visible = false
        longPress.stop()
        if (visible) {
            scroller.stop()
            scrollX = 0
        }
    }

    NumberAnimation {
        id: scroller
        target: panel
        property: "scrollX"
    }

    // Under the grid: Jolla's keyAt takes the topmost child.
    Item {
        width: panel.width
        height: panel.gridHeight
        clip: true
        Repeater {
            model: panel.groups.length
            Rectangle {
                readonly property real at: panel.groupX[index] - panel.groupGap / 2 - panel.scrollX
                visible: panel.separated(index) && at > -width && at < panel.width + width
                x: Math.round(at - width / 2)
                width: Math.max(1, Math.round(Theme.pixelRatio))
                height: Math.round(panel.gridHeight * 0.85)
                y: Math.round((panel.gridHeight - height) / 2)
                gradient: Gradient {
                    GradientStop { position: 0; color: "transparent" }
                    GradientStop { position: 0.5; color: Theme.rgba(Theme.primaryColor, 0.2) }
                    GradientStop { position: 1; color: "transparent" }
                }
            }
        }
    }

    // Where the view is within the current group. Under the grid, as above.
    Rectangle {
        id: bar
        readonly property real groupX: panel.groupX[panel.currentGroup]
        readonly property real groupWidth: (panel.groupStart[panel.currentGroup + 1]
                                            - panel.groupStart[panel.currentGroup]) * panel.cellWidth
        readonly property real track: panel.width - 2 * Theme.paddingMedium
        readonly property real position: Math.max(0, Math.min(1,
            (panel.scrollX - groupX) / Math.max(1, groupWidth - panel.width)))
        visible: groupWidth > panel.width + 1
        width: Math.max(Theme.paddingLarge, track * panel.width / Math.max(1, groupWidth))
        height: Math.max(2, Math.round(3 * Theme.pixelRatio))
        radius: height / 2
        x: Theme.paddingMedium + position * (track - width)
        y: panel.gridHeight - height - Math.round(Theme.paddingSmall / 2)
        color: Theme.rgba(Theme.primaryColor, 0.4)
    }

    Item {
        id: grid
        width: panel.width
        height: panel.gridHeight
        clip: true

        // The slots: one more column than fits, for the column cut at an edge.
        readonly property int slotColumns: panel.columns + 1

        Repeater {
            model: (panel._grid.columns + 1) * panel._grid.rows
            EmojiKey {
                readonly property int slotColumn: Math.floor(index / panel.rows)
                readonly property int row: index % panel.rows
                readonly property int column: panel.firstColumn
                    + ((slotColumn - panel.firstColumn % grid.slotColumns) + grid.slotColumns) % grid.slotColumns
                readonly property int group: panel.groupAt(column)
                x: panel.columnX(column) - panel.scrollX
                y: row * panel.cellHeight
                width: panel.cellWidth
                height: panel.cellHeight
                base: panel.emojiAt(column, row)
                emoji: panel.groups[group].variants && panel.picked[base] ? panel.picked[base] : base
                variants: panel.groups[group].variants ? (EmojiData.variants[base] || "") : ""
                removable: group === 0 && base !== ""
                longPressHint: panel.longPressHint
            }
        }
    }

    // The touches on the grid (see the top).
    property Item _touchArea: null
    property int _pointId: -1
    property bool _sliding: false
    property real _startX
    property real _startY
    property real _startScroll
    property real _lastX
    property real _lastTime
    property real _velocity: 0   // pixels per ms, smoothed
    property int _stopPointId: -1   // a touch that stopped a flick: types nothing
    property Item _pressKey: null    // the key with variants under the finger
    property real _fingerX           // the finger, in the grid
    property real _fingerY
    property string _menuBase        // the emoji whose menu is open
    property bool _menuRemovable: false   // a recent emoji's menu

    Component.onCompleted: _touchArea = typeof keyboard !== "undefined" ? _findTouchArea(keyboard, 0) : null

    Connections {
        target: panel.visible ? panel._touchArea : null
        onPressed: panel._pressed(touchPoints)
        onUpdated: panel._updated(touchPoints)
        onReleased: panel._released(touchPoints, false)
        onCanceled: panel._released(touchPoints, true)
    }

    // Jolla's keyboard adds a touch after this item hears of it: the touch
    // that stopped a flick is cancelled right after.
    Timer {
        id: stopTouch
        interval: 0
        onTriggered: {
            if (panel._stopPointId !== -1)
                keyboard.cancelTouchPoint(panel._stopPointId)
            panel._stopPointId = -1
        }
    }

    function _findTouchArea(item, depth) {
        var children = item ? item.children : null
        if (!children || depth > 2)
            return null
        for (var i = 0; i < children.length; ++i) {
            if (children[i].maximumTouchPoints !== undefined && children[i].mouseEnabled !== undefined)
                return children[i]
        }
        for (i = 0; i < children.length; ++i) {
            var found = _findTouchArea(children[i], depth + 1)
            if (found)
                return found
        }
        return null
    }

    function _find(points) {
        for (var i = 0; i < points.length; ++i) {
            if (points[i].pointId === _pointId)
                return points[i]
        }
        return null
    }

    function _pressed(points) {
        if (_pointId !== -1 || !current)
            return
        for (var i = 0; i < points.length; ++i) {
            var p = grid.mapFromItem(_touchArea, points[i].x, points[i].y)
            if (p.x >= 0 && p.x < grid.width && p.y >= 0 && p.y < grid.height) {
                _pointId = points[i].pointId
                _startX = p.x
                _startY = p.y
                _lastX = p.x
                _lastTime = Date.now()
                _velocity = 0
                _sliding = false
                _fingerX = p.x
                _fingerY = p.y
                keyboard.interactive = false   // no swipe to another layout
                if (scroller.running) {
                    scroller.stop()
                    _stopPointId = _pointId
                    stopTouch.restart()
                } else {
                    _pressKey = _variantKeyAt(p.x, p.y)
                    if (_pressKey)
                        longPress.restart()
                }
                return
            }
        }
    }

    function _updated(points) {
        var point = _pointId !== -1 ? _find(points) : null
        if (!point)
            return
        var p = grid.mapFromItem(_touchArea, point.x, point.y)
        _fingerX = p.x
        _fingerY = p.y
        if (emojiMenu.visible) {
            _moveMenu(p.x, p.y)
            return
        }
        var now = Date.now()
        if (!_sliding) {
            var key = _variantKeyAt(p.x, p.y)
            if (key !== _pressKey) {   // onto another key: its own long press
                _pressKey = key
                if (key)
                    longPress.restart()
                else
                    longPress.stop()
            }
            var dx = p.x - _startX
            if (Math.abs(dx) < Theme.startDragDistance || Math.abs(dx) < Math.abs(p.y - _startY))
                return
            longPress.stop()
            _sliding = true
            keyboard.cancelTouchPoint(_pointId)   // types nothing
            _startX = p.x
            _startScroll = scrollX
        } else if (now > _lastTime) {
            // Smoothed over about 50 ms: touch events can come close together.
            var ms = now - _lastTime
            _velocity += ((p.x - _lastX) / ms - _velocity) * ms / (ms + 50)
        }
        _lastX = p.x
        _lastTime = now
        scrollX = Math.max(0, Math.min(maxScroll, _startScroll - (p.x - _startX)))
    }

    function _released(points, canceled) {
        if (_pointId === -1 || !_find(points))
            return
        longPress.stop()
        _pressKey = null
        if (emojiMenu.visible) {
            var picked = canceled ? "" : emojiMenu.activeEmoji
            emojiMenu.visible = false
            _pointId = -1
            if (canceled)
                keyboard.interactive = true
            if (picked === emojiMenu.removeCell) {
                panel.recentRemoved(_menuBase)
            } else if (picked !== "") {
                inputKey.text = picked
                keyboard.inputHandler._handleKeyPress(inputKey)
                keyboard.inputHandler._handleKeyClick(inputKey)
                keyboard.inputHandler._handleKeyRelease(inputKey)
                if (!_menuRemovable)
                    panel.variantPicked(_menuBase, picked)
            }
            return
        }
        // A flick goes on, slowing down (OutQuad: it starts at the finger's
        // speed when it travels half its duration times that speed).
        if (_sliding && !canceled && Date.now() - _lastTime < 100 && Math.abs(_velocity) > 0.3)
            scrollTo(scrollX - _velocity * 400, 800)
        if (canceled)
            keyboard.interactive = true
        _pointId = -1
        _sliding = false
    }

    // The key with a long-press menu at (x, y) in the grid, or null.
    function _variantKeyAt(x, y) {
        var key = grid.childAt(x, y)
        return key && typeof key.variants === "string" && key.enabled
               && (key.variants !== "" || key.removable) ? key : null
    }

    // The key is let go of in Jolla's keyboard, so that nothing is typed.
    function _openMenu() {
        var key = _pressKey
        if (_pointId === -1 || !key)
            return
        keyboard.cancelTouchPoint(_pointId)
        var rows = key.removable ? [[key.emoji, emojiMenu.removeCell]]
                                 : key.variants.split("|").map(function(r) { return r.split(" ") })
        _menuRemovable = key.removable
        var row = 0
        var column = 0
        for (var r = 0; r < rows.length; ++r) {
            var c = rows[r].indexOf(key.emoji)
            if (c >= 0) {
                row = r
                column = c
                break
            }
        }
        emojiMenu.rows = rows
        // Where the current variant's cell goes: under the finger, or (one
        // row) one cell above the key.
        var single = rows.length === 1
        var targetX = single ? key.x + key.width / 2 : _fingerX
        var targetY = single ? key.y - cellHeight / 2 : _fingerY
        var cellX = emojiMenu.padding + (column + 0.5) * cellWidth
        var cellY = emojiMenu.padding + (row + 0.5) * cellHeight
        emojiMenu.x = Math.max(0, Math.min(width - emojiMenu.width, targetX - cellX))
        emojiMenu.y = Math.max(0, Math.min(height - emojiMenu.height, targetY - cellY))
        _menuBase = key.base
        _moveMenu(_fingerX, _fingerY)
        emojiMenu.visible = true
    }

    // The active cell: the one under the finger (grid and panel coordinates
    // are the same).
    function _moveMenu(x, y) {
        var rows = emojiMenu.rows
        var column = Math.floor((x - emojiMenu.x - emojiMenu.padding) / cellWidth)
        var row = rows.length === 1 ? 0 : Math.floor((y - emojiMenu.y - emojiMenu.padding) / cellHeight)
        row = Math.max(0, Math.min(rows.length - 1, row))   // above or below: the nearest row
        if (column < 0 || column >= rows[row].length) {
            emojiMenu.activeRow = -1   // off its sides: nothing
            return
        }
        emojiMenu.activeRow = row
        emojiMenu.activeColumn = column
    }

    Timer {
        id: longPress
        interval: 500
        onTriggered: panel._openMenu()
    }

    // Types the menu's pick (as Jolla's Popper does).
    KeyBase {
        id: inputKey
        keyType: KeyType.PopupKey
        visible: false
    }

    // The groups, Backspace. (Not called `bottom`: inside the Repeater's
    // keys that name is their own anchor line.)
    Item {
        id: groupRow
        y: panel.height - panel.keyHeight
        width: panel.width
        height: panel.keyHeight
        readonly property real endWidth: panel.width * 0.14
        // Split landscape has no suggestion bar: a key back to the letters.
        readonly property real startWidth: panel.keyboardKey ? endWidth : 0
        readonly property real groupWidth: (width - startWidth - endWidth) / panel.groups.length

        FunctionKey {
            visible: panel.keyboardKey
            width: groupRow.startWidth
            height: groupRow.height
            icon.source: "image://theme/icon-m-keyboard"
            background.visible: false   // as Backspace, at the other end
            onClicked: panel.closeRequested()
        }

        Repeater {
            model: panel.groups.length
            FunctionKey {
                id: groupKey
                x: groupRow.startWidth + index * groupRow.groupWidth
                width: groupRow.groupWidth
                height: groupRow.height
                background.visible: panel.currentGroup === index
                background.anchors.margins: Theme.paddingSmall / 2   // around the icon
                onClicked: panel.scrollTo(panel.groupX[index], 250)
                Image {
                    anchors.centerIn: parent
                    readonly property int side: Math.round(Math.min(groupKey.width, groupKey.height) * 0.7)
                    width: side
                    height: side
                    sourceSize.width: side
                    sourceSize.height: side
                    asynchronous: true
                    source: side > 0 ? EmojiUtil.source(panel.groups[index].icon) : ""
                }
            }
        }

        BackspaceKey {
            x: groupRow.width - groupRow.endWidth
            width: groupRow.endWidth
            height: groupRow.height
        }
    }

    EmojiMenu {
        id: emojiMenu
        visible: false
        cellWidth: panel.cellWidth
        cellHeight: panel.cellHeight
    }
}
