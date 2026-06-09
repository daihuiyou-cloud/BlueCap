#include "RecordModeCard.h"

#include "paint/PaintMetrics.h"
#include "paint/PaintPrimitives.h"
#include "utils/IconHelper.h"

#include <QEvent>
#include <QPainter>

RecordModeCard::RecordModeCard(RecordMode mode, const QString &title,
                               const QString &subtitle, const QString &iconPath,
                               QWidget *parent)
    : QAbstractButton(parent)
    , m_mode(mode)
    , m_title(title)
    , m_subtitle(subtitle)
    , m_iconPath(iconPath)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(190, paint::Metrics::modeCardHeight);
    m_palette = paint::theme(false);
    recolorIcon();
}

void RecordModeCard::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_palette = paint::theme(dark);
    recolorIcon();
    update();
}

void RecordModeCard::recolorIcon()
{
    if (m_iconPath.isEmpty())
        return;
    const QColor accent = m_mode == RecordMode::Region
        ? m_palette.accentGreen
        : (m_mode == RecordMode::Window ? m_palette.accentPurple : m_palette.primary);
    m_iconCache = icon::renderSvg(m_iconPath, accent, 36);
}

QSize RecordModeCard::sizeHint() const
{
    return QSize(216, paint::Metrics::modeCardHeight);
}

void RecordModeCard::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void RecordModeCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void RecordModeCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto &pal = m_palette;
    const bool checked = isChecked();
    const bool disabled = !isEnabled();
    const QRectF r = QRectF(rect());

    QColor bg = checked ? pal.cardCheckedBg : (m_hovered ? pal.cardHoverBg : pal.cardBg);
    QColor border = checked ? pal.cardCheckedBorder : pal.cardBorder;
    if (disabled) {
        bg.setAlpha(120);
        border = pal.hairline;
    }

    paint::drawCard(p, r, bg, border, paint::Metrics::cardRadius, checked ? 1.4 : 1.0);
    paint::drawVerticalSheen(p, r, paint::Metrics::cardRadius, m_darkMode);

    const QColor accent = m_mode == RecordMode::Region
        ? pal.accentGreen
        : (m_mode == RecordMode::Window ? pal.accentPurple : pal.primary);

    if (!m_iconCache.isNull()) {
        if (disabled) {
            p.save();
            p.setOpacity(0.45);
            p.drawPixmap((width() - 36) / 2, 26, m_iconCache);
            p.restore();
        } else {
            p.drawPixmap((width() - 36) / 2, 26, m_iconCache);
        }
    }

    QFont titleFont = p.font();
    titleFont.setPixelSize(17);
    titleFont.setBold(true);
    p.setFont(titleFont);
    paint::drawElidedText(p, QRect(18, 72, width() - 36, 24),
                          Qt::AlignCenter, m_title,
                          disabled ? pal.faintText : pal.text);

    QFont subFont = p.font();
    subFont.setPixelSize(12);
    subFont.setBold(false);
    p.setFont(subFont);
    paint::drawElidedText(p, QRect(18, 96, width() - 36, 20),
                          Qt::AlignCenter, m_subtitle,
                          disabled ? pal.faintText : pal.mutedText);

    if (checked) {
        p.setPen(Qt::NoPen);
        p.setBrush(pal.primary);
        const qreal bx = width() - 31;
        const qreal by = 13;
        const qreal bd = 22;
        p.drawEllipse(QRectF(bx, by, bd, bd));
        p.setPen(QPen(Qt::white, 2));
        qreal cx = bx + bd / 2.0;
        qreal cy = by + bd / 2.0;
        p.drawLine(QPointF(cx - 5, cy), QPointF(cx - 2, cy + 3));
        p.drawLine(QPointF(cx - 2, cy + 3), QPointF(cx + 4, cy - 3));
    }

    if (hasFocus())
        paint::drawFocusRing(p, r, pal.primary, paint::Metrics::focusRadius);
}
