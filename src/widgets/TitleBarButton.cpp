#include "TitleBarButton.h"
#include "utils/IconHelper.h"
#include "theme/ThemeColors.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

TitleBarButton::TitleBarButton(const QString &iconPath, const QString &tooltip,
                               bool isCloseButton, QWidget *parent)
    : QPushButton(parent)
    , m_iconPath(iconPath)
    , m_closeButton(isCloseButton)
{
    setFixedSize(30, 30);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tooltip);
    updateIcon();
}

void TitleBarButton::setIconPath(const QString &path)
{
    m_iconPath = path;
    updateIcon();
}

void TitleBarButton::setDarkMode(bool dark)
{
    m_darkMode = dark;
    updateIcon();
    update();
}

void TitleBarButton::updateIcon()
{
    const auto &tc = ThemeColors::forMode(m_darkMode).titleBar;
    if (!m_iconPath.isEmpty())
        setIcon(icon::coloredIcon(m_iconPath, 20, tc.normal, tc.active, tc.disabled));
}

void TitleBarButton::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void TitleBarButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    m_pressed = false;
    update();
    QPushButton::leaveEvent(event);
}

void TitleBarButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
    QPushButton::mousePressEvent(event);
}

void TitleBarButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}

void TitleBarButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    bool disabled = !isEnabled();

    QColor bg = Qt::transparent;
    QColor fg = disabled ? a.titleBarButtonDisabled : a.titleBarButton;

    if (m_closeButton && m_hovered && !disabled) {
        bg = m_pressed ? a.closeButtonHoverBg.darker(120) : a.closeButtonHoverBg;
        fg = a.closeButtonHoverText;
    } else if (m_hovered && !disabled) {
        bg = m_pressed ? a.titleBarButtonHoverBg.darker(110) : a.titleBarButtonHoverBg;
        fg = a.titleBarButtonHover;
    }

    if (bg.alpha() > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(bg);
        painter.drawRoundedRect(QRectF(rect()), 14, 14);
    }

    if (!m_iconPath.isEmpty()) {
        QIcon::Mode mode = disabled ? QIcon::Disabled : QIcon::Normal;
        QPixmap px = icon().pixmap(20, 20, mode);
        if (!px.isNull()) {
            int x = (width() - 20) / 2;
            int y = (height() - 20) / 2;
            painter.drawPixmap(x, y, px);
        }
    }
}
