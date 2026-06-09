#include "StyledListWidget.h"
#include "PaintedScrollBar.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

StyledListWidget::StyledListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setAlternatingRowColors(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSpacing(8);
    setFrameShape(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);

    auto *sb = new PaintedScrollBar(this);
    sb->setFixedWidth(10);
    setVerticalScrollBar(sb);
}

void StyledListWidget::setDarkMode(bool dark)
{
    m_darkMode = dark;
    if (auto *sb = qobject_cast<PaintedScrollBar *>(verticalScrollBar()))
        sb->setDarkMode(dark);
    viewport()->update();
    update();
}

void StyledListWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);

    painter.setPen(QPen(a.videoListBorder, 1));
    painter.setBrush(a.videoListBg);
    painter.drawPath(path);

    painter.setClipRect(rect().adjusted(1, 1, -1, -1));
    QListWidget::paintEvent(event);
}
