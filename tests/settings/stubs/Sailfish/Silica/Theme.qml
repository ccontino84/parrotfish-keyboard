pragma Singleton
import QtQuick 2.15
QtObject {
    enum ColorScheme { LightOnDark, DarkOnLight }
    readonly property real pixelRatio: 1.75
    readonly property real paddingSmall: 6 * pixelRatio
    readonly property real paddingMedium: 12 * pixelRatio
    readonly property real paddingLarge: 24 * pixelRatio
    readonly property real horizontalPageMargin: 24 * pixelRatio
    readonly property real buttonWidthMedium: 292 * pixelRatio
    readonly property real itemSizeSmall: 80 * pixelRatio
    readonly property real itemSizeMedium: 100 * pixelRatio
    readonly property real fontSizeExtraSmall: 20 * pixelRatio
    readonly property real fontSizeSmall: 24 * pixelRatio
    readonly property real fontSizeMedium: 32 * pixelRatio
    readonly property real fontSizeLarge: 40 * pixelRatio
    readonly property real fontSizeExtraLarge: 50 * pixelRatio
    readonly property string fontFamily: "sans"
    readonly property color primaryColor: "white"
    readonly property color secondaryColor: "gray"
    readonly property color highlightColor: "cyan"
    readonly property color secondaryHighlightColor: "teal"
    readonly property color highlightBackgroundColor: "blue"
    readonly property color errorColor: "red"
    readonly property real highlightBackgroundOpacity: 0.3
    function rgba(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
}
