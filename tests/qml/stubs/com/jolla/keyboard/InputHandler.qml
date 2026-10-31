import QtQuick 2.15
// Stand-in for Jolla's InputHandler (com.jolla.keyboard): what handlers see.
Item {
    property Item pressedKey
    property Component topItem
    property Component verticalItem
    property bool active
    property QtObject palette: QtObject { property color highlightColor: "#7fbfff" }
    signal select(string text, int index)
    signal remove(string text, int index)
    signal paste(string text)
}
