// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-TBD
//
// The bottom row of the Parrotfish layouts. Based on SpacebarRow.qml of
// jolla-keyboard 0.10.4 (Jolla Ltd., BSD-3-Clause, see
// LICENSES/BSD-3-Clause.txt); changes: comma and period have a KeyBackground
// (the spacebar keeps Jolla's own, set apart from the letter keys); a long press on the comma
// offers : and ; as the period offers ! and ? (not where it types / or @),
// with the typing languages' signs added (the layout's punctuation: ¡ ¿,
// —);
// the backgrounds of ?123, the spacebar and Enter take the layout's gap;
// comma and period have a LongPressHint.
import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.keyboard 1.0
import "../.."

KeyboardRow {
    id: bottomRow

    splitIndex: 3

    // Jolla's backgrounds of ?123, the spacebar and Enter are inset by
    // paddingMedium; they get the layout's gap instead, as the other keys
    // (KeyBackground), so that the rows line up.
    Component.onCompleted: {
        for (var i = 0; i < children.length; ++i) {
            var key = children[i]
            for (var j = 0; j < key.children.length; ++j) {
                var item = key.children[j]
                if (item.radius !== undefined && item.styled === undefined && item.anchors
                        && item.anchors.margins === Theme.paddingMedium)
                    item.anchors.margins = Qt.binding(function() {
                        return bottomRow.parent && bottomRow.parent.keyGap !== undefined
                                ? bottomRow.parent.keyGap : Theme.paddingMedium
                    })
            }
        }
    }

    readonly property var _punctuation: parent && parent.punctuation !== undefined ? parent.punctuation : null

    SymbolKey {}
    ContextAwareCommaKey {
        property bool popupAlways: true   // as PeriodKey: also in the symbol view
        accents: caption !== "," ? "" : bottomRow._punctuation ? bottomRow._punctuation.comma : ":,;"
        accentsShifted: accents
        showHighlight: !commaBackground.styled
        KeyBackground { id: commaBackground }
        LongPressHint { frame: commaBackground }
    }
    SpacebarKey {}
    SpacebarKey {
        active: splitActive
        languageLabel: ""
    }
    PeriodKey {
        accents: bottomRow._punctuation ? bottomRow._punctuation.period : "!.?"
        accentsShifted: accents
        showHighlight: !periodBackground.styled
        KeyBackground { id: periodBackground }
        LongPressHint { frame: periodBackground }
    }
    EnterKey {}
}
