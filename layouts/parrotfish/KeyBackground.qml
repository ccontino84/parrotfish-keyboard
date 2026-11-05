// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// A subtle rounded background for a key of the Parrotfish layouts, in the
// ambience's colours. Put it inside a key (in a KeyboardRow of a layout).
// Fill and border are chosen separately, by the layout's keyFill and
// keyBorder:
// - fill "none", "flat", or "shaded" (lighter at the top, darker at the
//   bottom);
// - border "none", "line" (one colour), or "raised" (a rim lit from above:
//   light top edge, dark bottom edge, as on a raised button).
// Both "none" is Jolla's look. Letter keys stay fainter than the spacebar
// (Jolla's 0.07), so that it stands out: alphas of the ambience's primary
// colour, fill 0.04, shade 0.05, border 0.12 (for "raised", 0.2 of its
// white and black edges). functionLook (the sticky key) sets a key apart
// from the letters, whatever their look: flat, no border, as Jolla's
// function keys (Shift, Backspace, ?123, Enter: 0.17).
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: keyBackground

    readonly property Item key: parent
    readonly property Item layout: key && key.parent ? key.parent.parent : null
    property bool functionLook: false
    readonly property string fillStyle: functionLook ? "flat" : _setting("keyFill", ["none", "flat", "shaded"], "none")
    readonly property string borderStyle: functionLook ? "none" : _setting("keyBorder", ["none", "line", "raised"], "none")
    readonly property bool styled: fillStyle !== "none" || borderStyle !== "none"
    readonly property bool raised: borderStyle === "raised"
    readonly property var strengths: _strengths(fillStyle, borderStyle)
    readonly property int borderWidth: Math.max(1, Math.round(Theme.pixelRatio))
    // Lighter at the top: with light text (dark ambience) more alpha is lighter.
    readonly property real _shadeSign: key.palette.colorScheme === Theme.LightOnDark ? 1 : -1
    readonly property real _topAlpha: Math.max(0, strengths.fill + _shadeSign * strengths.shade / 2)
    readonly property real _bottomAlpha: Math.max(0, strengths.fill - _shadeSign * strengths.shade / 2)
    // The rim of "raised" is drawn by a shader; plain border if it fails.
    // Qt 5 can report Error with an empty log for a shader that works.
    readonly property bool _rimShader: raised && !(rim.status === ShaderEffect.Error && rim.log !== "")

    // The layout's value of `name` if it is one of `values`, else `fallback`.
    function _setting(name, values, fallback) {
        var value = layout ? layout[name] : undefined
        return values.indexOf(value) >= 0 ? value : fallback
    }

    function _strengths(fill, border) {
        var result = { "fill": 0.04, "shade": 0.05, "border": border === "raised" ? 0.2 : 0.12 }
        if (functionLook)
            result.fill = 0.17
        if (fill === "none")
            result.fill = 0
        if (fill !== "shaded")
            result.shade = 0
        if (border === "none")
            result.border = 0
        return result
    }

    visible: styled
    z: -2   // below the label and Jolla's pressed highlight
    // The layout's gap all round (Jolla's spacebar and function keys get the
    // same, BottomRow); Jolla's paddings in other layouts.
    readonly property real _gap: layout && layout.keyGap !== undefined ? layout.keyGap : -1
    anchors {
        fill: parent
        // first and last keys of a row also own the row's indent
        leftMargin: key.leftPadding + (_gap >= 0 ? _gap : Theme.paddingSmall)
        rightMargin: key.rightPadding + (_gap >= 0 ? _gap : Theme.paddingSmall)
        topMargin: _gap >= 0 ? _gap : Theme.paddingMedium
        bottomMargin: _gap >= 0 ? _gap : Theme.paddingMedium
    }
    radius: geometry.keyRadius
    color: key.pressed ? Theme.rgba(key.palette.highlightBackgroundColor, 0.6)
                       : Theme.rgba(key.palette.primaryColor, strengths.fill)
    gradient: !key.pressed && strengths.shade !== 0 ? shadeGradient : null
    border.width: !key.pressed && !_rimShader && strengths.border > 0 ? borderWidth : 0
    border.color: Theme.rgba(key.palette.primaryColor, strengths.border)

    Gradient {
        id: shadeGradient
        GradientStop { position: 0; color: Theme.rgba(keyBackground.key.palette.primaryColor, keyBackground._topAlpha) }
        GradientStop { position: 1; color: Theme.rgba(keyBackground.key.palette.primaryColor, keyBackground._bottomAlpha) }
    }

    ShaderEffect {
        id: rim

        property point boxSize: Qt.point(width, height)
        property real cornerRadius: keyBackground.radius
        property real rimWidth: keyBackground.borderWidth
        property color lightColor: Qt.rgba(1, 1, 1, keyBackground.strengths.border)
        property color shadowColor: Qt.rgba(0, 0, 0, keyBackground.strengths.border)

        anchors.fill: parent
        visible: keyBackground.raised && !keyBackground.key.pressed
        blending: true

        // The distance to the rounded rectangle's edge gives the rim; the
        // edge's outward direction, against light from above (a little from
        // the left), mixes the light and the dark colour.
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform lowp float qt_Opacity;
            uniform highp vec2 boxSize;
            uniform highp float cornerRadius;
            uniform highp float rimWidth;
            uniform lowp vec4 lightColor;
            uniform lowp vec4 shadowColor;
            void main() {
                highp vec2 p = (qt_TexCoord0 - 0.5) * boxSize;
                highp vec2 q = abs(p) - (0.5 * boxSize - cornerRadius);
                highp float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - cornerRadius;
                highp vec2 n;
                if (q.x > 0.0 && q.y > 0.0)
                    n = normalize(q) * sign(p);
                else if (q.x > q.y)
                    n = vec2(sign(p.x), 0.0);
                else
                    n = vec2(0.0, sign(p.y));
                highp float lit = clamp(0.5 + 0.5 * dot(n, vec2(-0.3, -0.954)), 0.0, 1.0);
                highp float ring = clamp(0.5 - d, 0.0, 1.0) - clamp(0.5 - (d + rimWidth), 0.0, 1.0);
                // colour uniforms come premultiplied, as gl_FragColor must be
                gl_FragColor = mix(shadowColor, lightColor, lit) * ring * qt_Opacity;
            }"
    }
}
