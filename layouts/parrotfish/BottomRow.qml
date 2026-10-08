// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-TBD
//
// The bottom row of the Parrotfish layouts. Based on SpacebarRow.qml of
// jolla-keyboard 0.10.4 (Jolla Ltd., BSD-3-Clause, see
// LICENSES/BSD-3-Clause.txt); change: comma and period have a KeyBackground
// (the spacebar keeps Jolla's own, set apart from the letter keys).
import QtQuick 2.0
import com.jolla.keyboard 1.0
import "../.."

KeyboardRow {
    splitIndex: 3

    SymbolKey {}
    ContextAwareCommaKey {
        showHighlight: !commaBackground.styled
        KeyBackground { id: commaBackground }
    }
    SpacebarKey {}
    SpacebarKey {
        active: splitActive
        languageLabel: ""
    }
    PeriodKey {
        showHighlight: !periodBackground.styled
        KeyBackground { id: periodBackground }
    }
    EnterKey {}
}
