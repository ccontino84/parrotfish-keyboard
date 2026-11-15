// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// The spacebar's long press while voice input is on (the layout's
// voiceEnabled): Jolla's layout popup (LanguageSelectionPopup.qml: its look,
// sizes, cell rows and selection), with a microphone row below the layouts, selected when it
// opens. Lifting the finger there starts voice input; sliding onto a layout
// switches to it. Jolla's popup is kept from opening
// (keyboard.layoutChangeAllowed). Touches are read from Jolla's touch area
// as in emoji/EmojiPanel.qml; once the popup opens, the touch is cancelled
// in Jolla's keyboard, so that no space is typed.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Parrotfish.Keyboard 1.0

Item {
    id: menu

    property Item layout
    readonly property bool current: typeof keyboard !== "undefined" && keyboard.layout === layout
    readonly property bool enabledHere: layout && layout.voiceEnabled && current && keyboard.mode === "common"

    signal voiceRequested()

    readonly property int _voiceCell: -2
    readonly property bool _geometry: typeof geometry !== "undefined" && geometry.languageSelectionCellMargin !== undefined
    readonly property real _cellMargin: _geometry ? geometry.languageSelectionCellMargin : Theme.paddingMedium
    readonly property real _touchDelta: _geometry ? geometry.languageSelectionTouchDelta : Theme.paddingLarge
    readonly property real _initialDeltaSquared: _geometry ? geometry.languageSelectionInitialDeltaSquared : 400

    property Item _touchArea: null
    property Item _key: null
    property int _pointId: -1
    property bool _open: false
    property bool _moved: false
    property real _startX
    property real _startY
    property var _rows: []   // top to bottom: [{ cell, text, width }]
    property int _active: -1   // a layout's index in canvas.layoutModel, or _voiceCell

    onEnabledHereChanged: {
        _allowJollaMenu()
        if (!enabledHere && !_open) {
            hold.stop()
            _pointId = -1
        }
    }
    Component.onCompleted: {
        _touchArea = typeof keyboard !== "undefined" ? _findTouchArea(keyboard, 0) : null
        _allowJollaMenu()
    }
    Component.onDestruction: if (_took) _allowJollaMenu(true)

    // Jolla's menu is kept closed while this layout is in use; another
    // Parrotfish layout taking over keeps it closed (its own menu does).
    property bool _took: false
    function _allowJollaMenu(force) {
        if (typeof keyboard === "undefined" || !keyboard || keyboard.layoutChangeAllowed === undefined)
            return
        if (enabledHere && !force) {
            keyboard.layoutChangeAllowed = false
            _took = true
        } else if (_took && (force || !(keyboard.layout && keyboard.layout.arrangementName !== undefined))) {
            keyboard.layoutChangeAllowed = Qt.binding(function() { return keyboard.mode === "common" })
            _took = false
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
        for (var i = 0; i < points.length; ++i)
            if (points[i].pointId === _pointId)
                return points[i]
        return null
    }

    Connections {
        target: menu.enabledHere ? menu._touchArea : null
        onPressed: menu._pressed(touchPoints)
        onUpdated: menu._updated(touchPoints)
        onReleased: menu._released(touchPoints, false)
        onCanceled: menu._released(touchPoints, true)
    }

    Timer {
        id: hold
        interval: 500
        onTriggered: menu._openMenu()
    }

    // After the popup's first frames: starting the helper blocks briefly.
    Timer {
        id: prepare
        interval: 150
        onTriggered: Voice.prepare()
    }

    function _pressed(points) {
        if (_open || hold.running || layout.voiceView || layout.emojiView)
            return
        _pointId = -1
        for (var i = 0; i < points.length; ++i) {
            var k = keyboard.mapFromItem(_touchArea, points[i].x, points[i].y)
            var key = keyboard.keyAt(k.x, k.y)
            if (key && key.key === Qt.Key_Space) {
                _pointId = points[i].pointId
                _key = key
                _startX = points[i].x
                _startY = points[i].y
                hold.restart()
                return
            }
        }
    }

    function _updated(points) {
        var p = _pointId !== -1 ? _find(points) : null
        if (!p)
            return
        var dx = p.x - _startX, dy = p.y - _startY
        if (!_open) {
            if (Math.abs(dx) > Theme.startDragDistance || Math.abs(dy) > Theme.startDragDistance) {
                hold.stop()   // a slide or a swipe, not a long press
                _pointId = -1
            }
            return
        }
        if (!_moved && dx * dx + dy * dy < _initialDeltaSquared)
            return
        _moved = true
        // As Jolla's: the point counts a little above the finger, and the
        // selection stays when the finger is between two cells.
        var old = _active
        var inPopup = popup.mapFromItem(_touchArea, p.x, p.y)
        if (inPopup.y < 0)
            _active = -1
        else if (inPopup.y > popup.height)
            _active = _voiceCell   // the finger is still on the spacebar
        var q = content.mapFromItem(_touchArea, p.x, p.y - _touchDelta)
        var row = content.childAt(q.x, q.y)
        if (row && row.cells !== undefined) {
            var r = row.mapFromItem(content, q.x, q.y)
            var cell = row.childAt(r.x, r.y)
            if (cell && cell.cell !== undefined)
                _active = cell.cell
        }
        if (old !== _active && _active !== -1) {
            if (typeof SampleCache !== "undefined")
                SampleCache.play("/usr/share/sounds/jolla-ambient/stereo/keyboard_letter.wav")
            if (typeof buttonPressEffect !== "undefined")
                buttonPressEffect.play()
        }
    }

    function _released(points, canceled) {
        if (_pointId === -1 || !_find(points))
            return
        hold.stop()
        _pointId = -1
        if (!_open)
            return
        var opening = openAnimation.running
        _close()
        if (canceled || _active === -1)
            return
        if (_active === _voiceCell)
            menu.voiceRequested()
        else if (!opening && _active !== canvas.activeIndex)
            canvas.switchLayout(_active)
    }

    function _close() {
        _open = false
        if (_active !== _voiceCell)
            prepare.stop()
        keyboard.interactive = true
        if (openAnimation.running) {
            openAnimation.stop()
            popup.visible = false
        } else {
            fadeAnimation.start()
        }
        if (typeof canvas !== "undefined" && canvas.updateIMArea !== undefined)
            canvas.updateIMArea()
    }

    TextMetrics {
        id: metrics
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
    }

    function _openMenu() {
        if (_pointId === -1)
            return
        keyboard.cancelTouchPoint(_pointId)   // no space typed
        keyboard.interactive = false          // no swipe to another layout
        prepare.restart()                     // the model loads meanwhile
        if (typeof canvas !== "undefined" && canvas.captureFullScreen !== undefined)
            canvas.captureFullScreen()

        // Jolla's rows: filled from the last layout, so that the uneven row
        // is the topmost, each in the layouts' order.
        var maxAllowed = Math.min(menu.width - Theme.paddingSmall * 2,
                                  _geometry ? geometry.languageSelectionPopupMaxWidth : menu.width)
        var margins = _geometry ? geometry.languageSelectionPopupContentMargins : Theme.paddingLarge * 2
        var maxContent = maxAllowed - margins
        var model = canvas.layoutModel
        var chunks = [], chunk = [], rowWidth = 0, maxRowWidth = 0
        for (var i = model.count - 1; i >= 0; --i) {
            if (!model.get(i).enabled)
                continue
            var text = qsTrId(model.get(i).name)
            metrics.text = text
            var w = Math.ceil(metrics.advanceWidth) + _cellMargin * 2
            if (chunk.length && rowWidth + w > maxContent) {
                chunks.push(chunk)
                maxRowWidth = Math.max(maxRowWidth, rowWidth)
                chunk = []
                rowWidth = 0
            }
            chunk.unshift({ "cell": i, "text": text, "width": w })
            rowWidth += w
        }
        if (chunk.length) {
            chunks.push(chunk)
            maxRowWidth = Math.max(maxRowWidth, rowWidth)
        }
        var rows = chunks.reverse()
        var micWidth = Theme.iconSizeMedium + _cellMargin * 2
        rows.push([{ "cell": _voiceCell, "text": "", "width": micWidth }])
        maxRowWidth = Math.max(maxRowWidth, micWidth)
        _rows = rows
        _active = _voiceCell
        _moved = false

        popup.width = Math.min(maxAllowed, maxRowWidth + margins)
        var finger = menu.mapFromItem(_touchArea, _startX, _startY)
        if (menu.width - popup.width <= Theme.paddingLarge)
            popup.x = (menu.width - popup.width) / 2
        else
            popup.x = Math.min(menu.width - popup.width - Theme.paddingSmall,
                               Math.max(finger.x - popup.width / 2, Theme.paddingSmall))
        popup.bottomLine = (_key ? menu.mapFromItem(_key, 0, 0).y : finger.y) - Theme.paddingLarge
        popup.targetHeight = rows.length * Theme.itemSizeSmall
        fadeAnimation.stop()
        popup.height = 0
        popup.opacity = 1
        popup.visible = true
        _open = true
        openAnimation.start()
    }

    Rectangle {
        id: popup

        property real bottomLine
        property int targetHeight

        y: bottomLine - height
        z: 11
        visible: false
        radius: typeof geometry !== "undefined" && geometry.popperRadius !== undefined ? geometry.popperRadius : Theme.paddingSmall
        color: typeof keyboard !== "undefined" && keyboard.popperBackgroundColor !== undefined
               ? keyboard.popperBackgroundColor : Theme.highlightDimmerColor
        clip: openAnimation.running

        NumberAnimation on height {
            id: openAnimation
            running: false
            duration: 100
            easing.type: Easing.OutQuad
            to: popup.targetHeight + Theme.paddingLarge
        }

        SequentialAnimation {
            id: fadeAnimation
            NumberAnimation { target: popup; property: "opacity"; to: 0; duration: 100 }
            ScriptAction { script: popup.visible = false }
        }

        Column {
            id: content
            width: parent.width
            anchors.verticalCenter: parent.verticalCenter

            Repeater {
                model: menu._rows
                Row {
                    readonly property var cells: modelData
                    x: (content.width - width) / 2
                    height: Theme.itemSizeSmall

                    Repeater {
                        model: modelData
                        SilicaItem {
                            id: cellItem
                            readonly property int cell: modelData.cell
                            readonly property bool active: menu._active === cell
                            readonly property bool voice: cell === menu._voiceCell
                            width: modelData.width
                            height: Theme.itemSizeSmall

                            Label {
                                id: label
                                anchors.centerIn: parent
                                visible: !cellItem.voice
                                text: modelData.text
                                color: cellItem.active ? cellItem.palette.primaryColor : cellItem.palette.secondaryColor
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeMedium
                                font.bold: cellItem.active
                            }
                            Icon {
                                id: mic
                                anchors.centerIn: parent
                                visible: cellItem.voice
                                width: Theme.iconSizeMedium
                                height: width
                                sourceSize { width: width; height: height }
                                source: "image://theme/icon-m-mic"
                                opacity: cellItem.active ? 1 : 0.6
                            }
                            Rectangle {
                                anchors { top: cellItem.voice ? mic.bottom : label.bottom; horizontalCenter: parent.horizontalCenter }
                                width: cellItem.voice ? mic.width : label.paintedWidth
                                height: Math.max(1, Math.round(typeof Theme.dp === "function" ? Theme.dp(2) : 2))
                                color: cellItem.palette.primaryColor
                                visible: cellItem.active
                            }
                        }
                    }
                }
            }
        }
    }
}
