// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Voice input's panel, in place of the layout's rows. Touches are read from
// Jolla's touch area as in ../emoji/EmojiPanel.qml; with no keys here,
// Jolla's keyboard types nothing.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

Item {
    id: panel

    property Item _touchArea: null
    Component.onCompleted: _touchArea = typeof keyboard !== "undefined" ? _findTouchArea(keyboard, 0) : null

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

    // Pausing for a held button counts as paused.
    readonly property bool _canEdit: Voice.state === "listening" || Voice.state === "paused"
                                     || (Voice.state === "finishing" && _resume)
    readonly property bool _selected: Voice.selectionStart >= 0
    // With a selection, the undo button deletes it.
    readonly property bool _canUndo: _canEdit && (_selected || Voice.openText !== "" || Voice.undoFrom >= 0)
    readonly property bool _canFinish: _canEdit
    readonly property bool _canPause: _canEdit

    // What the finger pressed: "undo", "mic", "done", "text" or "".
    property string _pressed: ""
    property int _pointId: -1
    property real _pressX
    property real _pressY
    property bool _dragged: false
    property int _anchor: -1          // the word the selection started on
    property bool _tapOnSelected: false
    // Listening when the text, undo or done was pressed: paused while held.
    property bool _resume: false
    property var _lastPoint: null

    function _controlAt(point) {
        var controls = [undoButton, micButton, doneButton]
        for (var i = 0; i < controls.length; ++i) {
            var p = controls[i].mapFromItem(_touchArea, point.x, point.y)
            if (p.x >= 0 && p.x < controls[i].width && p.y >= 0 && p.y < controls[i].height)
                return controls[i].name
        }
        return ""
    }

    function _find(points) {
        for (var i = 0; i < points.length; ++i)
            if (points[i].pointId === _pointId)
                return points[i]
        return null
    }

    // The word index at a point (with `nearest`, also above or below the
    // lines); -1 for none or for the replacement being said.
    function _wordAt(point, nearest) {
        var p = flow.mapFromItem(_touchArea, point.x, point.y)
        var best = null, bestRow = -1, bestDx = 0
        var items = flow.children
        var rowGap = Infinity
        for (var i = 0; i < items.length; ++i) {
            var it = items[i]
            if (it.wordIndex === undefined || !it.visible)
                continue
            var dy = p.y < it.y ? it.y - p.y : (p.y > it.y + it.height ? p.y - it.y - it.height : 0)
            if (dy > 0 && !nearest)
                continue
            var dx = p.x < it.x ? it.x - p.x : (p.x > it.x + it.width ? p.x - it.x - it.width : 0)
            if (dy < rowGap || (dy === rowGap && dx < bestDx)) {
                rowGap = dy
                bestDx = dx
                best = it
            }
        }
        return best ? best.wordIndex : -1
    }

    function _selectTo(point) {
        var w = _wordAt(point, true)
        if (w < 0)
            return
        if (_anchor < 0)
            _anchor = w
        Voice.select(_anchor, w)
    }

    // Nothing said while the finger is down is dictated.
    function _holdPause() {
        _resume = Voice.state === "listening"
        if (_resume)
            Voice.pause()
    }

    function _endHold() {
        if (_resume)
            Voice.start()
        _resume = false
    }

    function _textPressed(point) {
        _dragged = false
        _holdPause()
        _anchor = _wordAt(point, false)
        _tapOnSelected = _anchor >= 0 && Voice.selectionStart === _anchor && Voice.selectionEnd === _anchor
        if (_anchor >= 0)
            Voice.select(_anchor, _anchor)
    }

    function _textReleased() {
        scroller.stop()
        if (!_dragged && (_anchor < 0 || _tapOnSelected))
            Voice.clearSelection()
        _endHold()
    }

    onVisibleChanged: {
        _pressed = ""
        _pointId = -1
        _scroll = 0
        scroller.stop()
        if (visible)
            wave.clear()
    }

    Connections {
        target: panel.visible ? panel._touchArea : null
        onPressed: {
            if (panel._pointId !== -1)
                return
            for (var i = 0; i < touchPoints.length; ++i) {
                var p = panel.mapFromItem(panel._touchArea, touchPoints[i].x, touchPoints[i].y)
                if (p.x >= 0 && p.x < panel.width && p.y >= 0 && p.y < panel.height) {
                    keyboard.interactive = false   // no swipe to another layout
                    panel._pointId = touchPoints[i].pointId
                    panel._pressX = touchPoints[i].x
                    panel._pressY = touchPoints[i].y
                    panel._resume = false
                    panel._pressed = panel._controlAt(touchPoints[i])
                    if ((panel._pressed === "undo" && panel._canUndo) || (panel._pressed === "done" && panel._canFinish))
                        panel._holdPause()
                    if (panel._pressed === "" && panel._canEdit && Voice.text !== "") {
                        panel._pressed = "text"
                        panel._textPressed(touchPoints[i])
                    }
                    return
                }
            }
        }
        onUpdated: {
            var p = panel._find(touchPoints)
            if (!p)
                return
            if (panel._pressed === "text") {
                if (!panel._dragged && Math.abs(p.x - panel._pressX) < Theme.startDragDistance
                        && Math.abs(p.y - panel._pressY) < Theme.startDragDistance)
                    return
                panel._dragged = true
                panel._lastPoint = { "x": p.x, "y": p.y }
                panel._selectTo(p)
                var t = textArea.mapFromItem(panel._touchArea, p.x, p.y)
                if (t.y < panel._lineHeight || t.y > textArea.height - panel._lineHeight)
                    scroller.start()
                else
                    scroller.stop()
            } else if (panel._pressed !== "" && panel._controlAt(p) !== panel._pressed) {
                panel._pressed = ""   // slid off: nothing happens on release
                panel._endHold()
            }
        }
        onReleased: {
            var p = panel._find(touchPoints)
            if (!p)
                return
            var control = panel._pressed
            panel._pressed = ""
            panel._pointId = -1
            if (control === "text") {
                panel._textReleased()
            } else if (control === "undo" && panel._canUndo) {
                if (panel._selected)
                    Voice.deleteSelection()
                else
                    Voice.undo()
                panel._endHold()
            } else if (control === "mic" && Voice.state === "paused") {
                Voice.start()
            } else if (control === "mic" && Voice.state === "listening") {
                Voice.pause()
            } else if (control === "done" && panel._canFinish) {
                panel._resume = false
                Voice.stop()
            } else {
                panel._endHold()
            }
        }
        onCanceled: {
            if (panel._find(touchPoints)) {
                if (panel._pressed === "text")
                    panel._textReleased()
                else
                    panel._endHold()
                panel._pressed = ""
                panel._pointId = -1
            }
        }
    }

    // Dragging a selection to the top or bottom edge scrolls the text.
    property real _scroll: 0   // how far up from the end the text is scrolled
    readonly property real _maxScroll: Math.max(0, flow.height - textArea.height)
    readonly property real _lineHeight: Theme.fontSizeLarge * 1.3
    on_MaxScrollChanged: if (_scroll > _maxScroll) _scroll = _maxScroll
    Timer {
        id: scroller
        interval: 50
        repeat: true
        onTriggered: {
            if (!panel._lastPoint)
                return
            var t = textArea.mapFromItem(panel._touchArea, panel._lastPoint.x, panel._lastPoint.y)
            var step = panel._lineHeight / 4
            panel._scroll = t.y < panel._lineHeight ? Math.min(panel._maxScroll, panel._scroll + step)
                                                     : Math.max(0, panel._scroll - step)
            panel._selectTo(panel._lastPoint)
        }
    }
    // New text brings the end back into view.
    Connections {
        target: Voice
        onTextChanged: if (panel._pressed !== "text" && !panel._selected) panel._scroll = 0
    }

    // The words to show, each with its look (fixed, open, selected, removed, restored).
    readonly property var _items: {
        var words = Voice.words
        var from = Voice.selectionStart, to = Voice.selectionEnd
        var preview = _pressed === "undo" && _canUndo
        var removeSelected = preview && _selected
        var removeOpen = preview && !_selected && Voice.openText !== ""
        var removeFrom = preview && !_selected && !removeOpen ? Voice.undoFrom : -1
        var removeTo = removeFrom >= 0 ? Voice.undoTo : -1
        var restores = removeFrom >= 0 ? Voice.undoRestores : []
        var addRestores = function() {
            for (var j = 0; j < restores.length; ++j)
                items.push({ "text": restores[j], "index": -1, "kind": "restored", "joined": false })
        }
        var open = Voice.openText === "" ? [] : Voice.openText.split(" ")
        var inPlace = Voice.replacing && open.length > 0
        var items = []
        var addOpen = function(first) {
            for (var j = 0; j < open.length; ++j)
                items.push({ "text": open[j], "index": first >= 0 ? first + j : -1,
                             "kind": removeOpen ? "removed" : "open", "joined": false })
        }
        for (var i = 0; i < words.length; ++i) {
            var selected = from >= 0 && i >= from && i <= to
            if (inPlace && i === from)
                addOpen(-1)
            if (inPlace && selected)
                continue
            if (i === removeTo)
                addRestores()
            items.push({ "text": words[i], "index": i,
                         "kind": selected ? (removeSelected ? "removed" : "selected")
                                 : i >= removeFrom && i < removeTo ? "removed" : "fixed",
                         "joined": selected && i < to })
        }
        if (removeTo === words.length)
            addRestores()
        if (!inPlace)
            addOpen(words.length)
        return items
    }

    // The text, its last lines when it doesn't fit.
    Item {
        id: textArea
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: controls.top
                  leftMargin: Theme.horizontalPageMargin; rightMargin: Theme.horizontalPageMargin
                  topMargin: Theme.paddingMedium }
        clip: true

        TextMetrics {
            id: space
            font.pixelSize: Theme.fontSizeLarge
            text: " "
        }

        Flow {
            id: flow
            width: parent.width
            y: Math.min(0, parent.height - height) + panel._scroll

            Repeater {
                model: panel._items
                Item {
                    readonly property int wordIndex: modelData.index
                    readonly property string kind: modelData.kind
                    width: word.width + space.advanceWidth
                    height: panel._lineHeight

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        x: -space.advanceWidth / 4
                        width: word.width + space.advanceWidth / 2 + (modelData.joined ? space.advanceWidth / 2 : 0)
                        height: parent.height
                        visible: kind === "selected"
                        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                    }
                    Label {
                        id: word
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.text
                        font.pixelSize: Theme.fontSizeLarge
                        font.strikeout: kind === "removed"
                        color: kind === "open" || kind === "removed" ? Theme.secondaryColor
                             : kind === "selected" || kind === "restored" ? Theme.highlightColor
                             : Theme.primaryColor
                    }
                }
            }
        }

        Label {
            anchors.centerIn: parent
            visible: Voice.text === ""
            color: Theme.secondaryHighlightColor
            text: Voice.state === "finishing" ? qsTr("Finishing…")
                  : !Voice.ready ? qsTr("Loading the model…")
                  : Voice.state === "paused" ? qsTr("Paused")
                  : qsTr("Speak")
        }
    }

    Label {
        anchors { bottom: controls.top; horizontalCenter: parent.horizontalCenter }
        visible: panel._selected && (Voice.state === "paused" || Voice.state === "listening")
        color: Theme.secondaryHighlightColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: Voice.state === "paused" ? qsTr("Tap the mic to replace") : qsTr("Say the replacement")
    }

    // The level, rising from the screen's bottom edge (below the layout's padding).
    VoiceWave {
        id: wave
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                  bottomMargin: panel.parent && panel.parent.bottomPadding !== undefined ? -panel.parent.bottomPadding : 0 }
        height: Theme.itemSizeSmall
        bottomAligned: true
        count: 93   // ~3 s: the helper sends a level every 32 ms
        lineWidth: Math.max(2, Math.round(Theme.paddingSmall / 3))
        color: Theme.rgba(Theme.highlightColor, 0.6)
        fillColor: Theme.rgba(Theme.highlightColor, 0.45)
    }

    Connections {
        target: Voice
        // The level floor is the room's noise: a flat line in silence.
        onLevelChanged: if (Voice.state === "listening") wave.push(Math.max(0, (Voice.level - 0.25) / 0.75))
    }

    // Undo, the microphone and done, each a third of the width.
    Row {
        id: controls
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                  bottomMargin: Theme.paddingSmall }
        height: Theme.iconSizeLarge * 1.4

        Item {
            id: undoButton
            readonly property string name: "undo"
            width: parent.width / 3
            height: parent.height

            Rectangle {
                anchors.centerIn: parent
                width: Theme.itemSizeMedium
                height: width
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                visible: panel._pressed === "undo" && panel._canUndo
            }
            // The theme has no undo icon.
            Icon {
                anchors.centerIn: parent
                source: panel._selected ? "image://theme/icon-m-backspace" : "image://theme/icon-m-rotate-left"
                highlighted: panel._pressed === "undo"
                opacity: panel._canUndo ? 1 : 0.4
            }
        }

        Item {
            id: micButton
            readonly property string name: "mic"
            width: parent.width / 3
            height: parent.height

            // The level behind the button (the room's noise is the floor).
            Rectangle {
                anchors.centerIn: parent
                width: micBackground.width * (1.08 + 0.45 * Math.max(0, (Voice.level - 0.25) / 0.75))
                height: width
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity * 0.6)
                visible: Voice.state === "listening"
                Behavior on width { NumberAnimation { duration: 90 } }
            }
            Rectangle {
                id: micBackground
                anchors.centerIn: parent
                width: Theme.iconSizeLarge * 1.3
                height: width
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor,
                                  panel._pressed === "mic" ? 2 * Theme.highlightBackgroundOpacity : Theme.highlightBackgroundOpacity)
            }
            // The theme's pause icon is a disc of its own: two bars instead.
            Icon {
                anchors.centerIn: parent
                width: Theme.iconSizeLarge
                height: width
                sourceSize { width: width; height: height }
                source: "image://theme/icon-m-mic"
                visible: Voice.state !== "listening"
                highlighted: panel._pressed === "mic"
                opacity: panel._canPause ? 1 : 0.4
            }
            // In the theme's line style: a stroke of 1/28 of the icon size.
            Row {
                anchors.centerIn: parent
                spacing: Math.round(Theme.iconSizeLarge * 0.16)
                visible: Voice.state === "listening"
                Repeater {
                    model: 2
                    Rectangle {
                        width: Math.max(2, Math.round(Theme.iconSizeLarge / 28))
                        height: Math.round(Theme.iconSizeLarge * 0.36)
                        radius: width / 2
                        color: panel._pressed === "mic" ? Theme.highlightColor : Theme.primaryColor
                    }
                }
            }
        }

        Item {
            id: doneButton
            readonly property string name: "done"
            width: parent.width / 3
            height: parent.height

            Rectangle {
                anchors.centerIn: parent
                width: Theme.itemSizeMedium
                height: width
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                visible: panel._pressed === "done" && panel._canFinish
            }
            Icon {
                anchors.centerIn: parent
                source: "image://theme/icon-m-accept"
                highlighted: panel._pressed === "done"
                opacity: panel._canFinish ? 1 : 0.4
            }
        }
    }
}
