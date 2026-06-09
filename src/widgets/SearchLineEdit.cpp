#include "SearchLineEdit.h"
#include "theme/ThemeColors.h"

#include <QFocusEvent>
#include <QPainter>
#include <QPainterPath>

SearchLineEdit::SearchLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("搜索视频..."));
    setClearButtonEnabled(true);
    setFixedHeight(44);
    setFrame(false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setTextMargins(38, 0, 28, 0);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral("QLineEdit { background: transparent; border: none; }"));
}

void SearchLineEdit::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto &a = ThemeColors::forMode(dark).app;
    QPalette p = palette();
    p.setColor(QPalette::Text, a.videoSearchText);
    p.setColor(QPalette::Base, Qt::transparent);
    p.setColor(QPalette::PlaceholderText, a.settingsFormLabel);
    p.setColor(QPalette::Highlight, a.selectionBg);
    p.setColor(QPalette::HighlightedText, a.selectionText);
    setPalette(p);
    update();
}

void SearchLineEdit::focusInEvent(QFocusEvent *event)
{
    m_focused = true;
    update();
    QLineEdit::focusInEvent(event);
}

void SearchLineEdit::focusOutEvent(QFocusEvent *event)
{
    m_focused = false;
    update();
    QLineEdit::focusOutEvent(event);
}

void SearchLineEdit::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QLineEdit::enterEvent(event);
}

void SearchLineEdit::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QLineEdit::leaveEvent(event);
}

void SearchLineEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    bool disabled = !isEnabled();

    QColor bg, border;
    if (disabled) {
        bg = a.inputDisabledBg;
        border = a.inputBorder;
    } else if (m_focused) {
        bg = a.videoSearchFocusBg;
        border = a.videoSearchFocusBorder;
    } else if (m_hovered) {
        bg = a.videoSearchHoverBg;
        border = a.videoSearchHoverBorder;
    } else {
        bg = a.videoSearchBg;
        border = a.videoSearchBorder;
    }

    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);

    painter.setPen(QPen(border, 1));
    painter.setBrush(bg);
    painter.drawPath(path);

    QColor iconColor = a.videoSearchText;
    iconColor.setAlpha(130);
    painter.setPen(QPen(iconColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(19, 20), 6, 6);
    painter.drawLine(QPointF(23, 24), QPointF(29, 30));

    painter.end();
    QLineEdit::paintEvent(event);
}
