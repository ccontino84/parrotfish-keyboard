import QtQuick 2.15
// Silica's ListItem: a MouseArea with a context menu opened on long press.
MouseArea {
    property real contentHeight: 80
    property Item menu
    property bool highlighted: pressed
    property bool menuOpen: false
    function openMenu() { menuOpen = true }
    width: parent ? parent.width : 0
    height: contentHeight
}
