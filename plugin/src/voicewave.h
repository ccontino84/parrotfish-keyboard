// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QColor>
#include <QQuickItem>
#include <QVector>

// The microphone level as a line scrolling from right to left (QML VoiceWave):
// push() adds a level (0-1) at the right edge. Centred, the line swings up
// and down; bottomAligned, it rises from the bottom edge, and fillColor fills
// the area below it, fading out downwards.
class VoiceWave : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    // The levels shown across the width.
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(bool bottomAligned READ bottomAligned WRITE setBottomAligned NOTIFY bottomAlignedChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)

public:
    explicit VoiceWave(QQuickItem *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);
    qreal lineWidth() const { return m_lineWidth; }
    void setLineWidth(qreal width);
    int count() const { return m_count; }
    void setCount(int count);
    bool bottomAligned() const { return m_bottomAligned; }
    void setBottomAligned(bool aligned);
    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &color);

    Q_INVOKABLE void push(qreal level);
    Q_INVOKABLE void clear();

signals:
    void colorChanged();
    void lineWidthChanged();
    void countChanged();
    void bottomAlignedChanged();
    void fillColorChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
    QColor m_color = Qt::white;
    qreal m_lineWidth = 2;
    int m_count = 90;
    bool m_bottomAligned = false;
    QColor m_fillColor = Qt::transparent;
    QVector<float> m_levels;   // oldest first
    quint64 m_pushed = 0;      // keeps each level's up/down side while it scrolls
};
