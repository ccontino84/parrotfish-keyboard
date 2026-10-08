// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The Parrotfish layout: one keyboard for any Latin-script typing language;
// the engine picks the language, this file only gives the keys. Its letter
// keys come from parrotfish/arrangements.js, picked by dconf
// /apps/parrotfish-keyboard/layout (qwerty, qwertz). The rows are rebuilt
// when it changes. The keys can have subtle backgrounds (parrotfish/, dconf
// /apps/parrotfish-keyboard/key-fill, key-border).
import QtQuick 2.6
import Nemo.Configuration 1.0
import ".."
import "parrotfish"
import "parrotfish/arrangements.js" as Arrangements

KeyboardLayout {
    id: parrotfishLayout

    readonly property var arrangement: Arrangements.get(arrangementConfig.value)
    // Shown on the spacebar until the engine knows the language.
    readonly property string arrangementName: arrangement.name
    // Bumped when the keys are rebuilt: the handler then reads them again.
    property int keysRevision: 0

    // Key backgrounds (parrotfish/KeyBackground.qml): fill and border are
    // set separately; keyTuning overrides their strengths.
    property string keyFill: keyFillConfig.value
    property string keyBorder: keyBorderConfig.value
    property string keyTuning: keyTuningConfig.value

    // The keys this layout created, per letter row.
    property var _createdKeys: [[], [], []]

    splitSupported: true

    onArrangementChanged: _build()
    Component.onCompleted: _build()

    ConfigurationValue {
        id: arrangementConfig
        key: "/apps/parrotfish-keyboard/layout"   // qwerty, qwertz
        defaultValue: "qwerty"
    }

    ConfigurationValue {
        id: keyFillConfig
        key: "/apps/parrotfish-keyboard/key-fill"   // none, flat, shaded
        defaultValue: "none"
    }

    ConfigurationValue {
        id: keyBorderConfig
        key: "/apps/parrotfish-keyboard/key-border"   // none, line, raised
        defaultValue: "none"
    }

    ConfigurationValue {
        id: keyTuningConfig
        key: "/apps/parrotfish-keyboard/key-tuning"   // hidden: "fill=0.03 shade=0.05 border=0.1"
        defaultValue: ""
    }

    // The letter rows are filled by _build(), in order, so that the keys'
    // order (and the split index) is the arrangement's.
    KeyboardRow { id: row1 }
    KeyboardRow { id: row2 }
    KeyboardRow {
        id: row3
        ShiftKey {}
    }

    BottomRow {}

    Component {
        id: letterKeyComponent
        LetterKey {}
    }

    Component {
        id: backspaceKeyComponent
        BackspaceKey {}
    }

    function _build() {
        var rows = [row1, row2, row3]
        var created = []
        for (var r = 0; r < rows.length; ++r) {
            var old = _createdKeys[r]
            for (var i = 0; i < old.length; ++i) {
                old[i].parent = null   // out of the row now; destroy() is deferred
                old[i].destroy()
            }
            var spec = arrangement.rows[r]
            var keys = []
            for (i = 0; i < spec.keys.length; ++i) {
                var k = spec.keys[i]
                var key = letterKeyComponent.createObject(rows[r], {
                    "caption": k.c, "captionShifted": k.s,
                    "symView": k.y || "", "symView2": k.y2 || "",
                    "accents": k.a || "", "accentsShifted": k.A || ""
                })
                if (k.fit)
                    key.fontSizeMode = Text.HorizontalFit
                keys.push(key)
            }
            if (rows[r] === row3)
                keys.push(backspaceKeyComponent.createObject(row3))
            rows[r].splitIndex = spec.split
            created.push(keys)
        }
        _createdKeys = created
        updateSizes()
        keysRevision++
    }
}
