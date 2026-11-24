import QtQuick 2.15
// connman's manager: the default route from MobileData.route ("" offline).
QtObject { readonly property var defaultRoute: MobileData.route ? {"type": MobileData.route} : null }
