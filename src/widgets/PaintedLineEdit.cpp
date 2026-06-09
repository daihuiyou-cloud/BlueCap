#include "PaintedLineEdit.h"

#include "paint/PaintPrimitives.h"

#include <QFocusEvent>
#include <QPainter>

PaintedLineEdit::PaintedLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setMouseTracking(true);
    setFrame(false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setTextMargins(10, 0, 10, 0);
    setStyleSheet(QStringLiteral("QLineEdit { background: transparent; border: none; }"));
}

void PaintedLineEdit::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto pal = paint::theme(dark);
    QPalette p = palette();
    p.setColor(QPalette::Text, pal.text);
    p.setColor(QPalette::Base, Qt::transparent);
    p.setColor(QPalette::PlaceholderText, pal.faintText);
    p.setColor(QPalette::Highlight, pal.colors.app.selectionBg);
    p.setColor(QPalette::HighlightedText, pal.colors.app.selectionText);
    setPalette(p);
    update();
}

void PaintedLineEdit::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QLineEdit::enterEvent(event);
}

void PaintedLineEdit::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QLineEdit::leaveEvent(event);
}

void PaintedLineEdit::focusInEvent(QFocusEvent *event)
{
    m_focused = true;
    update();
    QLineEdit::focusInEvent(event);
}

void PaintedLineEdit::focusOutEvent(QFocusEvent *event)
{
    m_focused = false;
    update();
    QLineEdit::focusOutEvent(event);
}

void PaintedLineEdit::paintEvent(QPaintEvent *event)
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
    p.end();
    QLineEdit::paintEvent(event);
}
