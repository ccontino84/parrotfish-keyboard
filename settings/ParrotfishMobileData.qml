// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Whether the phone is online through mobile data (connman's default
// route). Loaded apart, so that the voice page works without MeeGo.Connman.
import QtQuick 2.0
import MeeGo.Connman 0.2

Item {
    readonly property bool mobileData: !!manager.defaultRoute && manager.defaultRoute.type === "cellular"

    NetworkManager {
        id: manager
    }
}
