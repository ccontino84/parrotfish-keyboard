import QtQuick 2.15
Item {
    property bool highlighted
    property QtObject palette: QtObject {
        property color primaryColor: "white"
        property color highlightBackgroundColor: "#3d8fd6"
        property color highlightColor: "#7fbfff"
        property int colorScheme: 0
    }
}
