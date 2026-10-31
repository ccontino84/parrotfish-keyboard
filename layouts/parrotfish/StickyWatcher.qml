// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// Tells the layout when a sign is picked from the sticky key's long press
// (picked(sign)), so that the key keeps it. Jolla's Popper types a pick
// through a key of its own (keyType PopupKey): it sets that key's text, then
// passes the key to the input handler, while its target is still the key
// that was held. Watching that text works in every text field, also where
// Jolla uses its own input handler instead of ours (passwords, addresses).
// The Popper is found among the keyboard's children (searched two levels
// deep): it is not an exported type.
import QtQuick 2.6
import com.jolla.keyboard 1.0

Item {
    id: watcher

    property var keyboardItem   // Jolla's keyboard (KeyboardBase)
    signal picked(string sign)

    property Item _popper: null
    property Item _popupKey: null

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

    Connections {
        target: watcher._popupKey
        onTextChanged: {
            var popper = watcher._popper
            if (popper && popper.target && popper.target.sticky === true && watcher._popupKey.text !== "")
                watcher.picked(watcher._popupKey.text)
        }
    }
}
