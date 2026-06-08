#include "RegionSelector.h"

#include <QApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

RegionSelector::RegionSelector(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);

    auto *screen = QGuiApplication::primaryScreen();
    setGeometry(screen->geometry());

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
}

void RegionSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_origin = event->pos();
        m_rubberBand->setGeometry(QRect(m_origin, QSize()));
        m_rubberBand->show();
        m_selecting = true;
    }
}

void RegionSelector::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selecting) {
        m_rubberBand->setGeometry(QRect(m_origin, event->pos()).normalized());
    }
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        m_rubberBand->hide();
        QRect region = QRect(m_origin, event->pos()).normalized();
        if (region.width() > 10 && region.height() > 10) {
            emit regionSelected(region);
        }
        close();
    }
}

void RegionSelector::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}

void RegionSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 80));
}
