#include "SidebarButton.h"
#include "paint/PaintMetrics.h"
#include "theme/ThemeColors.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

SidebarButton::SidebarButton(const QString &text, bool checked, QWidget *parent)
    : QPushButton(parent)
{
    setText(text);
    setIconSize(QSize(24, 24));
    setCheckable(true);
    setChecked(checked);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(paint::Metrics::sidebarButtonMinHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
}

void SidebarButton::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void SidebarButton::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void SidebarButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void SidebarButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
    QPushButton::mousePressEvent(event);
}

void SidebarButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}

void SidebarButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    bool disabled = !isEnabled();
    bool checked = isChecked();

    QColor textColor;
    QColor bgColor = Qt::transparent;

    if (disabled) {
        textColor = a.sidebarButtonDisabledText;
    } else if (checked) {
        textColor = a.sidebarButtonCheckedText;
        bgColor = m_pressed ? a.sidebarButtonCheckedBg.darker(110) : a.sidebarButtonCheckedBg;
    } else if (m_pressed) {
        textColor = a.sidebarButtonCheckedText;
        bgColor = a.sidebarButtonCheckedBg;
    } else if (m_hovered) {
        textColor = a.sidebarButtonHoverText;
        bgColor = a.sidebarButtonHoverBg;
    } else {
        textColor = a.sidebarButtonText;
    }

    if (bgColor.alpha() > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 16, 16);
    }

    // Active indicator strip (left 3px bar for checked state)
    if (checked && !disabled) {
        QRectF strip(0, (height() - 28) / 2.0, 3, 28);
        painter.setPen(Qt::NoPen);
        painter.setBrush(a.sidebarButtonCheckedText);
        painter.drawRoundedRect(strip, 2, 2);
    }

    QFont font = painter.font();
    font.setPixelSize(18);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(textColor);

    int iconSize = 24;
    int textLeft = 20 + iconSize + 12;
    QRect iconRect(20 + 3, (height() - iconSize) / 2, iconSize, iconSize);
    QRect textRect(textLeft + 3, 0, width() - textLeft - 19, height());

    if (!icon().isNull()) {
        QIcon::Mode mode = disabled ? QIcon::Disabled : QIcon::Normal;
        QPixmap px = icon().pixmap(iconSize, iconSize, mode);
        if (!px.isNull())
            painter.drawPixmap(iconRect, px);
    }

    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
}
