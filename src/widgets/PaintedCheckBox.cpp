#include "PaintedCheckBox.h"

#include "paint/PaintPrimitives.h"

#include <QPainter>

PaintedCheckBox::PaintedCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    m_palette = paint::theme(false);
}

void PaintedCheckBox::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_palette = paint::theme(dark);
    update();
}

void PaintedCheckBox::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QCheckBox::enterEvent(event);
}

void PaintedCheckBox::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QCheckBox::leaveEvent(event);
}

void PaintedCheckBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto &pal = m_palette;
    const bool disabled = !isEnabled();
    QRect box(0, (height() - 20) / 2, 20, 20);
    QColor border = disabled ? pal.colors.app.checkboxIndicatorDisabledBorder
                    : m_hovered ? pal.colors.app.checkboxIndicatorHoverBorder
                                : pal.colors.app.checkboxIndicatorBorder;
    QColor bg = isChecked() ? pal.colors.app.checkboxIndicatorChecked
                : disabled ? pal.colors.app.checkboxIndicatorDisabledBg
                           : pal.colors.app.checkboxIndicatorBg;
    paint::drawCard(p, QRectF(box), bg, border, 5);
    if (isChecked()) {
        p.setPen(QPen(Qt::white, 2));
        qreal cx = box.center().x();
        qreal cy = box.center().y();
        p.drawLine(QPointF(cx - 5, cy), QPointF(cx - 2, cy + 3));
        p.drawLine(QPointF(cx - 2, cy + 3), QPointF(cx + 4, cy - 3));
    }
    QFont f = p.font();
    f.setPixelSize(14);
    p.setFont(f);
    paint::drawElidedText(p, QRect(30, 0, width() - 30, height()),
                          Qt::AlignLeft | Qt::AlignVCenter, text(),
                          disabled ? pal.colors.app.checkboxDisabledText : pal.colors.app.checkboxText);
}
