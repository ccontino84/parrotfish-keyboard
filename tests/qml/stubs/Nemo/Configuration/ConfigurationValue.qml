import QtQuick 2.15
QtObject {
    property string key
    property var defaultValue
    property var value: Dconf.values[key] !== undefined ? Dconf.values[key] : defaultValue
}
