import QtQuick 2.15
// Like dconf: a value written lands in Dconf.values, and follows it again.
QtObject {
    id: config
    property string key
    property var defaultValue
    property var value: Dconf.values[key] !== undefined ? Dconf.values[key] : defaultValue
    onValueChanged: {
        var stored = Dconf.values[key] !== undefined ? Dconf.values[key] : defaultValue
        if (value === stored)
            return
        var values = Object.assign({}, Dconf.values)
        values[key] = value
        Dconf.values = values
        value = Qt.binding(function() { return Dconf.values[config.key] !== undefined ? Dconf.values[config.key] : config.defaultValue })
    }
}
