#pragma once

#include "paint/PaintTheme.h"

#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QString>

namespace paint {

inline QPainterPath roundedPath(const QRectF &rect, qreal radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

inline void drawCard(QPainter &p, const QRectF &rect, const QColor &bg,
                     const QColor &border, qreal radius, qreal borderWidth = 1.0)
{
    p.setPen(QPen(border, borderWidth));
    p.setBrush(bg);
    qreal hw = borderWidth / 2.0;
    p.drawPath(roundedPath(rect.adjusted(hw, hw, -hw, -hw), radius));
}

inline void drawVerticalSheen(QPainter &p, const QRectF &rect, qreal radius, bool dark)
{
    QLinearGradient g(rect.topLeft(), rect.bottomLeft());
    g.setColorAt(0.0, dark ? QColor(255, 255, 255, 18) : QColor(255, 255, 255, 85));
    g.setColorAt(0.42, QColor(255, 255, 255, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawPath(roundedPath(rect.adjusted(1, 1, -1, -1), radius - 1));
}

inline void drawFocusRing(QPainter &p, const QRectF &rect, const QColor &color, qreal radius)
{
    p.setPen(QPen(color, 1.8));
    p.setBrush(Qt::NoBrush);
    qreal hw = 1.8 / 2.0;
    p.drawPath(roundedPath(rect.adjusted(2 + hw, 2 + hw, -2 - hw, -2 - hw), radius));
}

inline void drawElidedText(QPainter &p, const QRectF &rect, int flags,
                           const QString &text, const QColor &color)
{
    QFontMetrics fm(p.font());
    p.setPen(color);
    QRect aligned = rect.toAlignedRect();
    p.drawText(aligned, flags, fm.elidedText(text, Qt::ElideRight, aligned.width()));
}

}
