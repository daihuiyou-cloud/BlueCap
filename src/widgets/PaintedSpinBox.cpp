#include "PaintedSpinBox.h"

#include "paint/PaintPrimitives.h"

#include <QFocusEvent>
#include <QLineEdit>
#include <QPainter>

PaintedSpinBox::PaintedSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setMouseTracking(true);
    setFrame(false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    if (auto *le = lineEdit()) {
        le->setFrame(false);
        le->setAutoFillBackground(false);
        le->setTextMargins(12, 0, 34, 0);
        le->setStyleSheet(QStringLiteral("QLineEdit { background: transparent; border: none; }"));
    }
}

void PaintedSpinBox::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto pal = paint::theme(dark);
    if (auto *le = lineEdit()) {
        QPalette p = le->palette();
        p.setColor(QPalette::Base, Qt::transparent);
        p.setColor(QPalette::Text, isEnabled() ? pal.colors.app.inputText : pal.colors.app.inputDisabledText);
        p.setColor(QPalette::Highlight, pal.colors.app.selectionBg);
        p.setColor(QPalette::HighlightedText, pal.colors.app.selectionText);
        le->setPalette(p);
    }
    update();
}

void PaintedSpinBox::enterEvent(QEvent *event) { m_hovered = true; update(); QSpinBox::enterEvent(event); }
void PaintedSpinBox::leaveEvent(QEvent *event) { m_hovered = false; update(); QSpinBox::leaveEvent(event); }
void PaintedSpinBox::focusInEvent(QFocusEvent *event) { m_focused = true; update(); QSpinBox::focusInEvent(event); }
void PaintedSpinBox::focusOutEvent(QFocusEvent *event) { m_focused = false; update(); QSpinBox::focusOutEvent(event); }

void PaintedSpinBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto pal = paint::theme(m_darkMode);
    QColor bg = isEnabled() ? pal.colors.app.inputBg : pal.colors.app.inputDisabledBg;
    QColor border = isEnabled() ? pal.colors.app.inputBorder : pal.hairline;
    if (m_focused) {
        bg = pal.colors.app.inputFocusBg;
        border = pal.colors.app.inputFocusBorder;
    } else if (m_hovered) {
        bg = pal.colors.app.inputHoverBg;
        border = pal.colors.app.inputHoverBorder;
    }
    paint::drawCard(p, QRectF(rect()), bg, border, 8);
    p.setPen(QPen(isEnabled() ? pal.mutedText : pal.faintText, 1.7));
    const int cx = width() - 18;
    const int topCy = height() / 2 - 7;
    const int bottomCy = height() / 2 + 7;
    p.drawLine(cx - 4, topCy + 2, cx, topCy - 2);
    p.drawLine(cx + 4, topCy + 2, cx, topCy - 2);
    p.drawLine(cx - 4, bottomCy - 2, cx, bottomCy + 2);
    p.drawLine(cx + 4, bottomCy - 2, cx, bottomCy + 2);
}
