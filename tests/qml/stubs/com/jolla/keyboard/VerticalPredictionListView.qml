import QtQuick 2.15
ListView {
    property Item handler
    property bool canRemove
    property bool showRemoveButton
    property real _buttonMargin
    property real _maximumLabelWidth
    signal predictionsChanged()
}
