// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The other accented letters of a key. A letter key's long-press menu shows
// the typing languages' letters, and ⋯ when the key has more
// (arrangements.js, accentMenus), drawn as › (MoreArrow). Lifting the
// finger on it opens this panel where the menu was, with ‹ and the other
// letters (the key's moreAccents); the key stays pressed while it is open.
// A tap on a letter types it and closes the panel; a tap on ‹ switches to
// the menu's letters (with › at the end again), › back; a tap anywhere else
// on the keyboard closes it and types nothing.
//
// Jolla's Popper closes on release and types a pick through a key of its
// own (keyType PopupKey): it sets that key's text, then passes the key to
// the input handler. A lift on ⋯ shows as that text (as StickyWatcher sees
// picks): the text is emptied at once, so that no handler types ⋯ (a key
// without text types nothing, in ours and in Jolla's). The panel types its
// picks through the same key and the active input handler, as the Popper
// does: ours, or Jolla's in password, e-mail and web address fields. It is
// a sibling of the Popper (the same coordinates), above it and above Jolla's
// touch area (z -1 there, "so it doesn't intercept events that would have
// been handled by an interactive item"): while it is open, its MouseAreas
// take the taps.
import QtQuick 2.6
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import com.meego.maliitquick 1.0
import "../.."
import "arrangements.js" as Arrangements

Item {
    id: panel

    property Item keyboardItem   // Jolla's keyboard (KeyboardBase)
    property Item layout         // the layout: only its keys open the panel
    readonly property bool opened: _key !== null

    property Item _popper: null
    property Item _popupKey: null
    property Item _key: null       // the key whose letters are shown
    property bool _shifted: false  // as the key's menu was
    property bool _more: true      // the other letters, else the menu's
    property int _active: -1       // the cell under the finger
    // The letters shown, with Arrangements.more for the arrow: first (‹)
    // with the other letters, last (›) with the menu's.
    readonly property var _cells: {
        if (!_key)
            return []
        return _more ? [Arrangements.more].concat((_shifted ? _key.moreAccentsShifted : _key.moreAccents).split(""))
                     : (_shifted ? _key.accentsShifted : _key.accents).split("")
    }

    parent: _popper ? _popper.parent : null
    anchors.fill: parent
    z: 11   // above the Popper (10)
    visible: opened

    onKeyboardItemChanged: _find()
    Component.onCompleted: _find()

    function _find() {
        _popper = _findPopper(keyboardItem, 0)
        _popupKey = null
        var children = _popper ? _popper.children : []
        for (var i = 0; i < children.length; ++i)
            if (children[i].keyType === KeyType.PopupKey)
                _popupKey = children[i]
    }

    // As in StickyWatcher: the Popper is not an exported type.
    function _findPopper(item, depth) {
        var children = item ? item.children : null
        if (!children || depth > 2)
            return null
        for (var i = 0; i < children.length; ++i) {
            var c = children[i]
            if (c.target !== undefined && typeof c.release === "function"
                    && typeof c.setActiveCell === "function")
                return c
        }
        for (i = 0; i < children.length; ++i) {
            var found = _findPopper(children[i], depth + 1)
            if (found)
                return found
        }
        return null
    }

    function _inLayout(item) {
        for (; item; item = item.parent)
            if (item === layout)
                return true
        return false
    }

    function open(key, x, y) {
        _shifted = keyboardItem.isShifted
        _more = true
        _active = -1
        _key = key
        box.y = y
        _place(x)
    }

    // At `x`, or as near as it fits on the keyboard (as the Popper).
    function _place(x) {
        if (parent)
            box.x = Math.max(geometry.popperMargin,
                             Math.min(x, parent.width - box.width - geometry.popperMargin))
    }

    function close() {
        var key = _key
        _key = null
        _active = -1
        if (key)
            key.pressed = false
    }

    function _cellAt(x) {
        var i = Math.floor((x - geometry.accentPopperMargin) / geometry.accentPopperCellWidth)
        return i >= 0 && i < _cells.length ? i : -1
    }

    function _setActive(i) {
        if (i === _active)
            return
        _active = i
        if (i >= 0) {
            if (typeof SampleCache !== "undefined")
                SampleCache.play("/usr/share/sounds/jolla-ambient/stereo/keyboard_letter.wav")
            if (typeof buttonPressEffect !== "undefined")
                buttonPressEffect.play()
        }
    }

    function _pick(i) {
        var c = _cells[i]
        if (c === Arrangements.more) {
            _more = !_more
            _active = -1
            return
        }
        close()
        var key = _popupKey
        var handler = keyboardItem ? keyboardItem.inputHandler : null
        if (!key || !handler)
            return
        key.text = c
        handler._handleKeyPress(key)
        handler._handleKeyClick(key)
        handler._handleKeyRelease(key)
    }

    Connections {
        target: panel._popupKey
        onTextChanged: {
            var key = panel._popper ? panel._popper.target : null
            if (panel._popupKey.text !== Arrangements.more || !key || typeof key.moreAccents !== "string"
                    || !panel._inLayout(key))
                return
            panel._popupKey.text = ""   // types nothing
            panel.open(key, panel._popper.x, panel._popper.y)
        }
    }

    // Jolla's keyboard releases the key after the Popper's release: it stays
    // pressed while the panel is open.
    Connections {
        target: panel._key
        onPressedChanged: if (panel._key && !panel._key.pressed) panel._key.pressed = true
    }

    // The Popper's cell for Arrangements.more shows a MoreArrow instead of
    // its text (the cells are made anew for every key).
    Connections {
        target: panel._popper
        onExpandedChanged: if (panel._popper.expanded) panel._drawArrows(panel._popper)
        onTargetChanged: panel._fixOrder()
    }

    // The Popper puts the key's own letter in the cell under the finger plus
    // half the key's width (Popper.qml, setup(): translatedX counts it
    // twice): on keys wider than two cells (landscape without split) it is a
    // cell to the right of the finger, the next letter is under it, and the
    // menu's order changes (u's ù u ⋯ became ù ⋯ u). For this layout's keys
    // the letter goes back to its place in the menu, under the finger as
    // the Popper placed the menu. Runs after the Popper's setup().
    function _fixOrder() {
        var popper = _popper
        var key = popper ? popper.target : null
        if (!key || !popper.hasAccents || !_inLayout(key))
            return
        var model = null
        for (var i = 0; i < popper.resources.length; ++i) {
            var r = popper.resources[i]
            if (r && typeof r.move === "function" && r.count !== undefined)
                model = r
        }
        var text = keyboardItem.isShifted ? key.accentsShifted : key.accents
        if (!model || model.count !== text.length || (keyboardItem.inSymView || keyboardItem.inSymView2))
            return   // combining characters, or a symbol view: as it is
        var want = text.indexOf(key.text)
        if (want < 0)
            want = Math.floor(model.count / 2)
        var have = popper.activeCell
        if (have === want || have < 0 || have >= model.count)
            return
        model.move(have, want, 1)
        popper.activeCell = want
        popper.initialActiveCell = want
        // The row as setup() places it for the active cell (before expanding).
        for (i = 0; i < popper.children.length; ++i) {
            var row = popper.children[i]
            if (row.spacing !== undefined && row.children !== undefined && row.layoutDirection !== undefined)
                row.x = popper.radius - want * geometry.accentPopperCellWidth
                        + Math.floor((popper.width - 2 * popper.radius - geometry.accentPopperCellWidth) * .5)
        }
    }

    function _drawArrows(item) {
        var children = item.children
        for (var i = 0; i < children.length; ++i) {
            var cell = children[i]
            if (cell.character === Arrangements.more && cell.textVisible !== undefined) {
                cell.textVisible = false
                arrowComponent.createObject(cell, {"cell": cell})
            } else if (cell.character === undefined) {
                _drawArrows(cell)
            }
        }
    }

    Component {
        id: arrowComponent
        MoreArrow {
            property Item cell
            active: cell !== null && cell.active
        }
    }

    Connections {
        target: panel.keyboardItem
        onLayoutChanged: panel.close()
        onInSymViewChanged: panel.close()
        onPortraitModeChanged: panel.close()
    }

    Connections {
        target: MInputMethodQuick
        onActiveChanged: panel.close()
    }

    // Elsewhere on the keyboard: closes, typing nothing.
    MouseArea {
        anchors.fill: parent
        onPressed: panel.close()
    }

    Rectangle {
        id: box

        color: keyboardItem ? keyboardItem.popperBackgroundColor : "black"
        radius: geometry.popperRadius
        height: geometry.popperHeight
        width: panel._cells.length * geometry.accentPopperCellWidth + 2 * geometry.accentPopperMargin
        onWidthChanged: panel._place(x)

        Row {
            x: geometry.accentPopperMargin
            height: parent.height

            Repeater {
                model: panel._cells
                Item {
                    width: geometry.accentPopperCellWidth
                    height: geometry.popperHeight
                    PopperCell {
                        visible: modelData !== Arrangements.more
                        character: visible ? modelData : ""
                        active: index === panel._active
                    }
                    MoreArrow {
                        visible: modelData === Arrangements.more
                        direction: index === 0 ? -1 : 1
                        active: index === panel._active
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: panel._setActive(panel._cellAt(mouse.x))
            onPositionChanged: panel._setActive(mouse.y >= 0 && mouse.y < height ? panel._cellAt(mouse.x) : -1)
            onReleased: {
                var i = panel._cellAt(mouse.x)
                if (i >= 0 && i === panel._active && mouse.y >= 0 && mouse.y < height)
                    panel._pick(i)
                else
                    panel._active = -1
            }
            onCanceled: panel._active = -1
        }
    }
}
