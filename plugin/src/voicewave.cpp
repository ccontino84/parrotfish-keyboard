// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "voicewave.h"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>

VoiceWave::VoiceWave(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void VoiceWave::setColor(const QColor &color)
{
    if (m_color == color)
        return;
    m_color = color;
    emit colorChanged();
    update();
}

void VoiceWave::setLineWidth(qreal width)
{
    if (qFuzzyCompare(m_lineWidth, width))
        return;
    m_lineWidth = width;
    emit lineWidthChanged();
    update();
}

void VoiceWave::setCount(int count)
{
    count = std::max(2, count);
    if (m_count == count)
        return;
    m_count = count;
    if (m_levels.size() > m_count)
        m_levels.remove(0, m_levels.size() - m_count);
    emit countChanged();
    update();
}

void VoiceWave::setBottomAligned(bool aligned)
{
    if (m_bottomAligned == aligned)
        return;
    m_bottomAligned = aligned;
    emit bottomAlignedChanged();
    update();
}

void VoiceWave::setFillColor(const QColor &color)
{
    if (m_fillColor == color)
        return;
    m_fillColor = color;
    emit fillColorChanged();
    update();
}

void VoiceWave::push(qreal level)
{
    m_levels.append(static_cast<float>(std::max<qreal>(0, std::min<qreal>(1, level))));
    if (m_levels.size() > m_count)
        m_levels.remove(0, m_levels.size() - m_count);
    ++m_pushed;
    update();
}

void VoiceWave::clear()
{
    m_levels.clear();
    update();
}

QSGNode *VoiceWave::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    // The fill below the line, then the line.
    QSGNode *root = old;
    if (!root) {
        root = new QSGNode;
        auto *fill = new QSGGeometryNode;
        auto *fillGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
        fillGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
        fill->setGeometry(fillGeometry);
        fill->setFlag(QSGNode::OwnsGeometry);
        fill->setMaterial(new QSGVertexColorMaterial);
        fill->setFlag(QSGNode::OwnsMaterial);
        root->appendChildNode(fill);
        auto *line = new QSGGeometryNode;
        auto *lineGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        lineGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
        line->setGeometry(lineGeometry);
        line->setFlag(QSGNode::OwnsGeometry);
        line->setMaterial(new QSGFlatColorMaterial);
        line->setFlag(QSGNode::OwnsMaterial);
        root->appendChildNode(line);
    }
    auto *fill = static_cast<QSGGeometryNode *>(root->firstChild());
    auto *line = static_cast<QSGGeometryNode *>(root->lastChild());
    auto *material = static_cast<QSGFlatColorMaterial *>(line->material());
    if (material->color() != m_color) {
        material->setColor(m_color);
        line->markDirty(QSGNode::DirtyMaterial);
    }

    // One half sine wave per level, the newest at the right edge (missing
    // levels are 0): alternately up and down, or all up from the bottom.
    constexpr int Sub = 6;
    const int n = m_count;
    const float step = float(width()) / (n - 1);
    const float half = float(m_lineWidth) / 2;
    const float base = m_bottomAligned ? float(height()) - half : float(height()) / 2;
    const float amp = std::max(0.0f, m_bottomAligned ? base - half : base - float(m_lineWidth));
    QVector<QPointF> points;
    points.reserve((n - 1) * Sub + 1);
    if (m_bottomAligned) {
        // Humps HumpSteps levels wide that scroll with the levels, as tall
        // as the levels under them (interpolated).
        constexpr float HumpSteps = 4;
        auto levelAt = [this](int back) {
            const int at = m_levels.size() - 1 - back;
            return at >= 0 && at < m_levels.size() ? m_levels.at(at) : 0.0f;
        };
        const int samples = (n - 1) * Sub;
        for (int j = 0; j <= samples; ++j) {
            const float x = float(j) / Sub * step;
            const float back = float(width() - x) / step;
            const int b0 = int(back);
            const float f = back - b0;
            const float level = levelAt(b0) * (1 - f) + levelAt(b0 + 1) * f;
            const float u = (float(m_pushed) - back) / HumpSteps;
            points.append(QPointF(x, base - amp * level * std::fabs(std::sin(float(M_PI) * u))));
        }
    }
    for (int i = 0; !m_bottomAligned && i < n - 1; ++i) {
        const int back = n - 2 - i;   // 0 = the newest level
        const int at = m_levels.size() - 1 - back;
        const float level = at >= 0 ? m_levels.at(at) : 0.0f;
        const float sign = m_bottomAligned || ((m_pushed - back) & 1) == 0 ? -1.0f : 1.0f;
        for (int k = 0; k < Sub; ++k) {
            const float t = float(k) / Sub;
            points.append(QPointF(i * step + t * step, base + sign * amp * level * std::sin(float(M_PI) * t)));
        }
    }
    if (!m_bottomAligned)
        points.append(QPointF(width(), base));

    QSGGeometry *geometry = line->geometry();
    geometry->allocate(2 * points.size());
    QSGGeometry::Point2D *v = geometry->vertexDataAsPoint2D();
    for (int i = 0; i < points.size(); ++i) {
        const QPointF a = points.at(std::max(0, i - 1)), b = points.at(std::min(points.size() - 1, i + 1));
        float dx = float(b.x() - a.x()), dy = float(b.y() - a.y());
        const float len = std::max(1e-3f, std::sqrt(dx * dx + dy * dy));
        dx /= len;
        dy /= len;
        const float x = float(points.at(i).x()), y = float(points.at(i).y());
        v[2 * i].set(x - dy * half, y + dx * half);
        v[2 * i + 1].set(x + dy * half, y - dx * half);
    }
    line->markDirty(QSGNode::DirtyGeometry);

    // The fill: fillColor at the line, transparent at the bottom edge
    // (the material takes premultiplied colours).
    QSGGeometry *fillGeometry = fill->geometry();
    const bool filled = m_bottomAligned && m_fillColor.alpha() > 0;
    fillGeometry->allocate(filled ? 2 * points.size() : 0);
    if (filled) {
        const float alpha = float(m_fillColor.alphaF());
        const auto r = uchar(m_fillColor.redF() * alpha * 255), g = uchar(m_fillColor.greenF() * alpha * 255),
                   b = uchar(m_fillColor.blueF() * alpha * 255), a = uchar(alpha * 255);
        QSGGeometry::ColoredPoint2D *c = fillGeometry->vertexDataAsColoredPoint2D();
        for (int i = 0; i < points.size(); ++i) {
            const float x = float(points.at(i).x());
            c[2 * i].set(x, float(points.at(i).y()), r, g, b, a);
            c[2 * i + 1].set(x, float(height()), 0, 0, 0, 0);
        }
    }
    fill->markDirty(QSGNode::DirtyGeometry);
    return root;
}
