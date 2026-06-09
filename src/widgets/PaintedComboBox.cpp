#include "PaintedComboBox.h"

#include "paint/PaintPrimitives.h"

#include <QAbstractItemView>
#include <QFocusEvent>
#include <QPainter>

PaintedComboBox::PaintedComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setMouseTracking(true);
    setFrame(false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet(QStringLiteral("QComboBox { background: transparent; border: none; }"));
}

void PaintedComboBox::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto pal = paint::theme(dark);
    if (view()) {
        QPalette vp = view()->palette();
        vp.setColor(QPalette::Window, pal.colors.app.comboPopupBg);
        vp.setColor(QPalette::WindowText, pal.colors.app.comboPopupText);
        vp.setColor(QPalette::Highlight, pal.colors.app.comboPopupSelectionBg);
        vp.setColor(QPalette::HighlightedText, pal.colors.app.comboPopupSelectionText);
        view()->setPalette(vp);
        view()->setAutoFillBackground(false);
        view()->setStyleSheet(QStringLiteral(
            "QAbstractItemView {"
            " background: %1;"
            " color: %2;"
            " border: 1px solid %3;"
            " outline: 0;"
            " selection-background-color: %4;"
            " selection-color: %5;"
            "}"
            "QAbstractItemView::item { min-height: 30px; padding: 5px 10px; }")
            .arg(pal.colors.app.comboPopupBg.name(QColor::HexArgb),
                 pal.colors.app.comboPopupText.name(QColor::HexArgb),
                 pal.colors.app.comboPopupBorder.name(QColor::HexArgb),
                 pal.colors.app.comboPopupSelectionBg.name(QColor::HexArgb),
                 pal.colors.app.comboPopupSelectionText.name(QColor::HexArgb)));
    }
    update();
}

void PaintedComboBox::enterEvent(QEvent *event) { m_hovered = true; update(); QComboBox::enterEvent(event); }
void PaintedComboBox::leaveEvent(QEvent *event) { m_hovered = false; update(); QComboBox::leaveEvent(event); }
void PaintedComboBox::focusInEvent(QFocusEvent *event) { m_focused = true; update(); QComboBox::focusInEvent(event); }
void PaintedComboBox::focusOutEvent(QFocusEvent *event) { m_focused = false; update(); QComboBox::focusOutEvent(event); }

void PaintedComboBox::paintEvent(QPaintEvent *)
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
    QFont f = p.font();
    f.setPixelSize(14);
    p.setFont(f);
    paint::drawElidedText(p, rect().adjusted(12, 0, -36, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, currentText(),
                          isEnabled() ? pal.colors.app.inputText : pal.colors.app.inputDisabledText);
    p.setPen(QPen(isEnabled() ? pal.mutedText : pal.faintText, 2));
    const int cx = width() - 20;
    const int cy = height() / 2;
    p.drawLine(cx - 4, cy - 2, cx, cy + 3);
    p.drawLine(cx + 4, cy - 2, cx, cy + 3);
}
