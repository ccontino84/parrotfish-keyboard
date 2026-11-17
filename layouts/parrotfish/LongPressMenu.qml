// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Our additions to Jolla's long-press menu (the Popper): the arrows to a
// key's other letters (› and ‹, after `dwell` ms), the key's own letter in
// its place (_fixOrder), and picked(sign) for the sticky key.
//
// The Popper isn't an exported type: it is found among the keyboard's
// children (two levels deep). Its cells are a ListModel ({labelText,
// inputText}), swapped here; release() types the active cell through a key
// of its own (PopupKey), passed to the input handler.
import QtQuick 2.6
import com.jolla.keyboard 1.0
import "arrangements.js" as Arrangements

Item {
    id: menu

    property Item keyboardItem   // Jolla's keyboard (KeyboardBase)
    property Item layout         // the layout: only its keys are changed
    property int dwell: 400
    signal picked(string sign)

    property Item _popper: null
    property Item _popupKey: null
    property Item _row: null       // the Popper's row of cells
    property var _cells: null      // the Popper's ListModel
    // While the other letters show: the menu's cells and the Popper's place.
    property var _saved: null
    // A switch was made: none until the finger leaves the arrow.
    property bool _switched: false

    onKeyboardItemChanged: _find()
    Component.onCompleted: _find()

    function _find() {
        _popper = _findPopper(keyboardItem, 0)
        _popupKey = null
        _row = null
        _cells = null
        if (!_popper)
            return
        var children = _popper.children
        for (var i = 0; i < children.length; ++i) {
            var c = children[i]
            if (c.keyType === KeyType.PopupKey)
                _popupKey = c
            else if (c.spacing !== undefined && c.layoutDirection !== undefined)
                _row = c
        }
        for (i = 0; i < _popper.resources.length; ++i) {
            var r = _popper.resources[i]
            if (r && typeof r.move === "function" && r.count !== undefined)
                _cells = r
        }
    }

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

    // The arrow in cell `i` (Arrangements.more or back), or "".
    function _arrowAt(i) {
        if (!_cells || i < 0 || i >= _cells.count)
            return ""
        var c = _cells.get(i).labelText
        return c === Arrangements.more || c === Arrangements.back ? c : ""
    }

    function _watchArrow() {
        var onArrow = _popper.expanded && _arrowAt(_popper.activeCell) !== ""
        if (!onArrow)
            _switched = false
        if (onArrow && !_switched)
            dwellTimer.restart()
        else
            dwellTimer.stop()
    }

    Timer {
        id: dwellTimer
        interval: menu.dwell
        onTriggered: menu._switch()
    }

    function _fill(cells) {
        _cells.clear()
        for (var i = 0; i < cells.length; ++i)
            _cells.append(typeof cells[i] === "string" ? {"labelText": cells[i], "inputText": cells[i]} : cells[i])
    }

    function _switch() {
        var popper = _popper
        var key = popper.target
        var arrow = _arrowAt(popper.activeCell)
        if (!popper.expanded || arrow === "" || !key || typeof key.moreAccents !== "string" || !_inLayout(key))
            return
        var cell = geometry.accentPopperCellWidth
        var margin = geometry.accentPopperMargin
        // The middle of the arrow's cell, where the finger is.
        var fingerX = popper.x + margin + (popper.activeCell + 0.5) * cell
        if (arrow === Arrangements.more) {
            var saved = []
            for (var i = 0; i < _cells.count; ++i)
                saved.push({"labelText": _cells.get(i).labelText, "inputText": _cells.get(i).inputText})
            _saved = {"cells": saved, "width": popper.expandedWidth, "x": popper.expandedX,
                      "initial": popper.initialActiveCell}
            var letters = (keyboardItem.isShifted ? key.moreAccentsShifted : key.moreAccents).split("")
            // ‹ where › was, the letters on its left, or first if they don't fit.
            var first = fingerX - (letters.length + 0.5) * cell - margin < geometry.popperMargin
            var at = first ? 0 : letters.length
            letters.splice(at, 0, Arrangements.back)
            _fill(letters)
            popper.expandedWidth = letters.length * cell + 2 * margin
            popper.expandedX = fingerX - (at + 0.5) * cell - margin
            popper.initialActiveCell = 1   // the expanded state places the Popper at expandedX
        } else if (_saved) {
            _fill(_saved.cells)
            popper.expandedWidth = _saved.width
            popper.expandedX = _saved.x
            popper.initialActiveCell = _saved.initial
            _saved = null
        } else {
            return
        }
        // The arrow if still under the finger, else nothing until it moves.
        var under = Math.floor((fingerX - popper.x - margin) / cell)
        _switched = true
        popper.activeCell = _arrowAt(under) !== "" ? under : -1
        _drawArrows(popper)
        if (typeof SampleCache !== "undefined")
            SampleCache.play("/usr/share/sounds/jolla-ambient/stereo/keyboard_letter.wav")
        if (typeof buttonPressEffect !== "undefined")
            buttonPressEffect.play()
    }

    Connections {
        target: menu._popper
        onTargetChanged: {
            dwellTimer.stop()
            menu._saved = null
            menu._switched = false
            menu._fixOrder()
        }
        onExpandedChanged: {
            if (menu._popper.expanded)
                menu._drawArrows(menu._popper)
            menu._watchArrow()
        }
        onActiveCellChanged: menu._watchArrow()
    }

    Connections {
        target: menu._popupKey
        onTextChanged: {
            var text = menu._popupKey.text
            if (text === Arrangements.more || text === Arrangements.back) {
                menu._popupKey.text = ""   // a key without text types nothing (ours and Jolla's handler)
                return
            }
            var key = menu._popper ? menu._popper.target : null
            if (key && key.sticky === true && text !== "")
                menu.picked(text)
        }
    }

    // Popper.qml's setup() counts half the key's width twice: on keys wider
    // than two cells the key's letter lands a cell off and the menu's order
    // changes. Puts it back; runs after setup().
    function _fixOrder() {
        var popper = _popper
        var key = popper ? popper.target : null
        if (!key || !popper.hasAccents || !_inLayout(key) || !_cells)
            return
        var text = keyboardItem.isShifted ? key.accentsShifted : key.accents
        if (_cells.count !== text.length || (keyboardItem.inSymView || keyboardItem.inSymView2))
            return   // combining characters, or a symbol view: as it is
        var want = text.indexOf(key.text)
        if (want < 0)
            want = Math.floor(_cells.count / 2)
        var have = popper.activeCell
        if (have === want || have < 0 || have >= _cells.count)
            return
        _cells.move(have, want, 1)
        popper.activeCell = want
        popper.initialActiveCell = want
        // The row as setup() places it for the active cell (before expanding).
        if (_row)
            _row.x = popper.radius - want * geometry.accentPopperCellWidth
                     + Math.floor((popper.width - 2 * popper.radius - geometry.accentPopperCellWidth) * .5)
    }

    // The arrows' cells show a MoreArrow (cells are made anew every time).
    function _drawArrows(item) {
        var children = item.children
        for (var i = 0; i < children.length; ++i) {
            var cell = children[i]
            if ((cell.character === Arrangements.more || cell.character === Arrangements.back)
                    && cell.textVisible !== undefined) {
                cell.textVisible = false
                arrowComponent.createObject(cell, {"cell": cell,
                                                   "direction": cell.character === Arrangements.back ? -1 : 1})
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
}
