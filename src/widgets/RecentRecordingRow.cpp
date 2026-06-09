#include "RecentRecordingRow.h"

#include "paint/PaintMetrics.h"
#include "paint/PaintPrimitives.h"

#include <QMouseEvent>
#include <QPainter>

RecentRecordingRow::RecentRecordingRow(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(paint::Metrics::recentRowHeight);
    setCursor(Qt::PointingHandCursor);
}

void RecentRecordingRow::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void RecentRecordingRow::setFileInfo(const QString &fileName, const QString &meta, const QString &duration)
{
    m_fileName = fileName;
    m_meta = meta;
    m_duration = duration;
    update();
}

QRect RecentRecordingRow::playRect() const { return QRect(width() - 116, 11, 32, 32); }
QRect RecentRecordingRow::folderRect() const { return QRect(width() - 70, 14, 26, 26); }
QRect RecentRecordingRow::menuRect() const { return QRect(width() - 32, 14, 22, 26); }

void RecentRecordingRow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (playRect().contains(event->pos())) {
            emit playRequested();
            return;
        }
        if (folderRect().contains(event->pos())) {
            emit openFolderRequested();
            return;
        }
        if (menuRect().contains(event->pos())) {
            emit menuRequested();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void RecentRecordingRow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto pal = paint::theme(m_darkMode);

    p.setPen(QPen(pal.hairline, 1));
    p.drawLine(12, height() - 1, width() - 12, height() - 1);

    const int rightActionsLeft = qMax(230, width() - 126);
    const QRect thumb(14, 8, 78, 38);
    const int textLeft = thumb.right() + 14;
    const int textWidth = qMax(80, rightActionsLeft - textLeft - 16);
    QLinearGradient tg(thumb.topLeft(), thumb.bottomRight());
    tg.setColorAt(0, QColor(78, 148, 255));
    tg.setColorAt(1, QColor(36, 48, 76));
    p.setPen(Qt::NoPen);
    p.setBrush(tg);
    p.drawRoundedRect(QRectF(thumb), 7, 7);
    p.setBrush(QColor(0, 0, 0, 140));
    p.drawRoundedRect(QRectF(thumb.right() - 34, thumb.bottom() - 16, 29, 13), 5, 5);
    QFont durationFont = p.font();
    durationFont.setPixelSize(10);
    durationFont.setBold(true);
    p.setFont(durationFont);
    p.setPen(Qt::white);
    p.drawText(QRect(thumb.right() - 34, thumb.bottom() - 17, 29, 15), Qt::AlignCenter, m_duration);

    QFont titleFont = p.font();
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    p.setFont(titleFont);
    paint::drawElidedText(p, QRect(textLeft, 8, textWidth, 20),
                          Qt::AlignLeft | Qt::AlignVCenter, m_fileName, pal.text);

    QFont metaFont = p.font();
    metaFont.setPixelSize(11);
    metaFont.setBold(false);
    p.setFont(metaFont);
    paint::drawElidedText(p, QRect(textLeft, 29, textWidth, 17),
                          Qt::AlignLeft | Qt::AlignVCenter, m_meta, pal.mutedText);

    QRect pr = playRect();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(pal.primary.red(), pal.primary.green(), pal.primary.blue(), 42));
    p.drawEllipse(pr);
    QPainterPath tri;
    tri.moveTo(pr.center().x() - 4, pr.center().y() - 7);
    tri.lineTo(pr.center().x() - 4, pr.center().y() + 7);
    tri.lineTo(pr.center().x() + 8, pr.center().y());
    tri.closeSubpath();
    p.setBrush(pal.primary);
    p.drawPath(tri);

    QRect fr = folderRect();
    p.setPen(QPen(pal.mutedText, 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(fr.adjusted(2, 7, -2, -3)), 3, 3);
    p.drawLine(fr.left() + 5, fr.top() + 9, fr.left() + 11, fr.top() + 9);

    QRect mr = menuRect();
    p.setBrush(pal.mutedText);
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 3; ++i)
        p.drawEllipse(QPointF(mr.center().x(), mr.top() + 7 + i * 6), 1.7, 1.7);
}
