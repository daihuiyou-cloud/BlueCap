#include "RegionSelector.h"

#include <QApplication>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

RegionSelector::RegionSelector(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::CrossCursor);

    cacheScreenLayout();

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
}

void RegionSelector::cacheScreenLayout()
{
    QRect virtualGeometry;
    const auto screens = QGuiApplication::screens();
    for (const auto *screen : screens) {
        virtualGeometry = virtualGeometry.united(screen->geometry());
    }
    setGeometry(virtualGeometry);
    m_screenLayoutValid = false;
}

ScreenLayout RegionSelector::screenForPoint(const QPoint &pt) const
{
    const auto screens = QGuiApplication::screens();
    for (const auto *screen : screens) {
        QRect geo = screen->geometry();
        if (geo.contains(pt))
            return { geo.topLeft(), geo.width(), geo.height() };
    }
    QRect primary = QGuiApplication::primaryScreen()->geometry();
    return { primary.topLeft(), primary.width(), primary.height() };
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
        m_currentPos = event->pos();
        m_rubberBand->setGeometry(QRect(m_origin, m_currentPos).normalized());
        update();
    }
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        m_rubberBand->hide();
        QRect region = QRect(m_origin, event->pos()).normalized();
        if (region.width() > 10 && region.height() > 10) {
            hide();
            emit regionSelected(region);
            close();
        } else {
            m_currentPos = event->pos();
            update();
        }
    }
}

void RegionSelector::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_selecting) {
            QRect region = QRect(m_origin, m_currentPos).normalized();
            if (region.width() > 10 && region.height() > 10) {
                m_rubberBand->hide();
                hide();
                emit regionSelected(region);
            }
            close();
        }
    }
}

void RegionSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Cache the screen containing the reference point so we don't iterate every frame
    QPoint refPoint = m_selecting ? QRect(m_origin, m_currentPos).center() : m_currentPos;
    if (refPoint.isNull())
        refPoint = m_origin;
    if (refPoint.isNull())
        refPoint = QGuiApplication::primaryScreen()->geometry().center();

    ScreenLayout screen = screenForPoint(mapToGlobal(refPoint));
    QPoint screenTopLeft = mapFromGlobal(screen.topLeft);

    // Draw dark overlay — only fill visible screen area (not the full virtual desktop)
    const auto screens = QGuiApplication::screens();
    for (const auto *s : screens) {
        QRect sg = s->geometry();
        QRect local = QRect(mapFromGlobal(sg.topLeft()), sg.size());
        painter.fillRect(local, QColor(0, 0, 0, 80));
    }

    // Pre-compute font metrics once
    static const QString hint = QStringLiteral("拖动鼠标选择录制区域 ｜ Enter 确认 ｜ Esc 取消");
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(hint);
    int x = screenTopLeft.x() + (screen.width - textWidth) / 2;
    int y = screenTopLeft.y() + screen.height - 60;

    painter.fillRect(x - 20, y - 34, textWidth + 40, 44, QColor(0, 0, 0, 140));
    painter.drawText(x, y, hint);

    if (m_selecting || (!m_selecting && !m_currentPos.isNull())) {
        QRect normalized = QRect(m_origin, m_currentPos).normalized();
        int w = normalized.width();
        int h = normalized.height();
        if (w < 10 || h < 10) {
            QString tooSmallHint = QStringLiteral("选择的区域太小（至少 10×10 像素）");
            QFont hintFont = painter.font();
            hintFont.setPointSize(14);
            hintFont.setBold(true);
            painter.setFont(hintFont);
            QFontMetrics hintFm(hintFont);
            int tw = hintFm.horizontalAdvance(tooSmallHint);
            int sx = screenTopLeft.x() + (screen.width - tw) / 2;
            int sy = screenTopLeft.y() + 60;
            painter.fillRect(sx - 24, sy - 40, tw + 48, 52, QColor(180, 40, 40, 200));
            painter.setPen(Qt::white);
            painter.drawText(sx, sy, tooSmallHint);
        } else {
            QString sizeText = QStringLiteral("%1 × %2").arg(w).arg(h);
            QFont sizeFont = painter.font();
            sizeFont.setPointSize(18);
            sizeFont.setBold(true);
            painter.setFont(sizeFont);
            QFontMetrics sizeFm(sizeFont);
            int sw = sizeFm.horizontalAdvance(sizeText);
            int sx = screenTopLeft.x() + (screen.width - sw) / 2;
            int sy = screenTopLeft.y() + 60;
            painter.fillRect(sx - 24, sy - 40, sw + 48, 52, QColor(0, 0, 0, 150));
            painter.setPen(Qt::white);
            painter.drawText(sx, sy, sizeText);
        }
    }
}
