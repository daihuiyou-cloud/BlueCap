#include "ActionButton.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>

ActionButton::ActionButton(const QString &text, Style style, QWidget *parent)
    : QPushButton(parent)
    , m_style(style)
{
    setText(text);
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    if (m_style == Reset || m_style == Browse) {
        setFixedHeight(46);
        setMinimumWidth(118);
    }
    setMouseTracking(true);
}

void ActionButton::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void ActionButton::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void ActionButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void ActionButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    bool disabled = !isEnabled();
    bool pressed = isDown();

    QColor bg, border, textColor;

    if (disabled) {
        bg = a.actionButtonDisabledBg;
        border = a.actionButtonBorder;
        textColor = a.actionButtonDisabledText;
    } else if (pressed) {
        bg = a.actionButtonPressedBg;
        border = a.actionButtonBorder;
        textColor = a.actionButtonHoverText;
    } else if (m_hovered) {
        bg = a.actionButtonHoverBg;
        border = a.actionButtonHoverBorder;
        textColor = a.actionButtonHoverText;
    } else {
        bg = a.actionButtonBg;
        border = a.actionButtonBorder;
        textColor = a.actionButtonText;
    }

    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);

    painter.setPen(QPen(border, 1));
    painter.setBrush(bg);
    painter.drawPath(path);

    if (hasFocus() && !disabled) {
        QPainterPath focusPath;
        focusPath.addRoundedRect(r.adjusted(-1, -1, 1, 1), 9, 9);
        painter.setPen(QPen(a.actionButtonFocusBorder, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(focusPath);
    }

    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(textColor);
    painter.drawText(r, Qt::AlignCenter, text());
}
