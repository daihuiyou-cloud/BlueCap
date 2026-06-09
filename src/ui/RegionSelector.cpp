#include "RegionSelector.h"
#include "theme/ThemeColors.h"

#include <QApplication>
#include <QFontMetrics>
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
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::CrossCursor);

    cacheScreenLayout();
    renderBackgroundCache();
}

void RegionSelector::cacheScreenLayout()
{
    m_screens = QGuiApplication::screens();
    QRect virtualGeometry;
    for (const auto *screen : m_screens)
        virtualGeometry = virtualGeometry.united(screen->geometry());
    setGeometry(virtualGeometry);
}

void RegionSelector::renderBackgroundCache()
{
    m_bgCache = QPixmap(size());
    m_bgCache.fill(Qt::transparent);

    QPainter p(&m_bgCache);
    for (const auto *s : m_screens) {
        QRect sg = s->geometry();
        QRect local = QRect(mapFromGlobal(sg.topLeft()), sg.size());
        p.fillRect(local, QColor(0, 0, 0, 90));
    }

    const QString hint = QStringLiteral("拖动鼠标选择录制区域  ·  Enter 确认  ·  Esc 取消");
    p.setPen(Qt::white);
    QFont font = p.font();
    font.setPixelSize(18);
    font.setBold(true);
    p.setFont(font);

    QFontMetrics fm(font);
    const int textWidth = fm.horizontalAdvance(hint);
    QScreen *primary = QGuiApplication::primaryScreen();
    QRect primaryGeo = primary->geometry();
    QPoint screenTopLeft = mapFromGlobal(primaryGeo.topLeft());
    const int x = screenTopLeft.x() + (primaryGeo.width() - textWidth) / 2;
    const int y = screenTopLeft.y() + primaryGeo.height() - 60;

    p.fillRect(x - 20, y - 28, textWidth + 40, 44, QColor(0, 0, 0, 150));
    p.drawText(x, y, hint);
}

ScreenLayout RegionSelector::screenForPoint(const QPoint &pt) const
{
    for (const auto *screen : m_screens) {
        QRect geo = screen->geometry();
        if (geo.contains(pt))
            return { geo.topLeft(), geo.width(), geo.height() };
    }
    QRect primary = m_screens.isEmpty() ? QRect() : m_screens.first()->geometry();
    return { primary.topLeft(), primary.width(), primary.height() };
}

void RegionSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_origin = event->pos();
        m_currentPos = m_origin;
        m_selecting = true;
        update();
    }
}

void RegionSelector::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selecting) {
        m_currentPos = event->pos();
        update();
    }
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
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
        QRect region = QRect(m_origin, m_currentPos).normalized();
        if (region.width() > 10 && region.height() > 10) {
            hide();
            emit regionSelected(region);
        }
        close();
    }
}

void RegionSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_bgCache);

    if (!m_selecting && m_currentPos.isNull())
        return;

    QRect normalized = QRect(m_origin, m_currentPos).normalized();
    const int w = normalized.width();
    const int h = normalized.height();

    QPoint refPoint = m_selecting ? normalized.center() : m_currentPos;
    if (refPoint.isNull())
        refPoint = m_origin;
    if (refPoint.isNull())
        return;

    ScreenLayout screen = screenForPoint(mapToGlobal(refPoint));
    QPoint screenTopLeft = mapFromGlobal(screen.topLeft);

    painter.setRenderHint(QPainter::Antialiasing);

    if (w < 10 || h < 10) {
        painter.fillRect(normalized, QColor(180, 40, 40, 160));
        const QString tooSmall = QStringLiteral("选择的区域太小（至少 10x10 像素）");
        painter.setPen(Qt::white);
        QFont f = painter.font();
        f.setPixelSize(18);
        f.setBold(true);
        painter.setFont(f);
        QFontMetrics fm(f);
        const int tw = fm.horizontalAdvance(tooSmall);
        const int sx = screenTopLeft.x() + (screen.width - tw) / 2;
        const int sy = screenTopLeft.y() + 60;
        painter.fillRect(sx - 24, sy - 40, tw + 48, 52, QColor(180, 40, 40, 210));
        painter.drawText(sx, sy, tooSmall);
        return;
    }
    const auto &a = ThemeColors::forMode(false).app;
    QColor selFill = a.inputFocusBorder;
    selFill.setAlpha(46);
    painter.fillRect(normalized, selFill);
    QColor selBorder = a.inputFocusBorder;
    selBorder.setAlpha(230);
    painter.setPen(QPen(selBorder, 2));
    painter.drawRect(normalized.adjusted(1, 1, -1, -1));

    QPen guide(QColor(255, 255, 255, 150), 1);
    guide.setStyle(Qt::DashLine);
    painter.setPen(guide);
    painter.drawLine(normalized.center().x(), normalized.top(), normalized.center().x(), normalized.bottom());
    painter.drawLine(normalized.left(), normalized.center().y(), normalized.right(), normalized.center().y());

    const QString sizeText = QStringLiteral("%1 x %2").arg(w).arg(h);
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setPixelSize(22);
    f.setBold(true);
    painter.setFont(f);
    QFontMetrics fm(f);
    const int sw = fm.horizontalAdvance(sizeText);
    const int sx = screenTopLeft.x() + (screen.width - sw) / 2;
    const int sy = screenTopLeft.y() + 60;
    painter.fillRect(sx - 24, sy - 36, sw + 48, 52, QColor(0, 0, 0, 155));
    painter.drawText(sx, sy, sizeText);
}
