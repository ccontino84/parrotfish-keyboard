pragma Singleton
import QtQuick 2.15
// Stand-in for Silica's Theme: base sizes scaled for a 1080 px wide phone
// (pixelRatio 1.75, an approximation; Silica computes the real values).
QtObject {
    enum ColorScheme { LightOnDark, DarkOnLight }
    readonly property real pixelRatio: 1.75
    readonly property real paddingSmall: 6 * pixelRatio
    readonly property real paddingMedium: 12 * pixelRatio
    readonly property real paddingLarge: 24 * pixelRatio
    readonly property real itemSizeSmall: 80 * pixelRatio
    readonly property real fontSizeLarge: 40 * pixelRatio
    readonly property real fontSizeExtraLarge: 50 * pixelRatio
    readonly property real fontSizeSmall: 24 * pixelRatio
    readonly property string fontFamily: "sans"
    readonly property real highlightBackgroundOpacity: 0.3
    function rgba(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
}
