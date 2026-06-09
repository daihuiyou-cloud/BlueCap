#include "VideoListItemWidget.h"
#include "paint/PaintPrimitives.h"
#include "theme/ThemeColors.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

VideoListItemWidget::VideoListItemWidget(const QString &path, const QString &title,
                                          const QString &meta, const QString &badgeText,
                                          const QPixmap &thumbnail, QWidget *parent)
    : QWidget(parent)
    , m_path(path)
    , m_title(title)
    , m_meta(meta)
    , m_badgeText(badgeText)
    , m_thumbnail(thumbnail)
{
    setMouseTracking(true);
}

void VideoListItemWidget::setThumbnail(const QPixmap &pixmap)
{
    m_thumbnail = pixmap;
    update();
}

void VideoListItemWidget::setSelected(bool selected)
{
    if (m_selected == selected) return;
    m_selected = selected;
    update();
}

void VideoListItemWidget::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void VideoListItemWidget::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void VideoListItemWidget::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void VideoListItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit openRequested(m_path);
    QWidget::mouseDoubleClickEvent(event);
}

void VideoListItemWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    QColor bg, border;
    if (m_selected) {
        bg = a.videoItemSelectedBg;
        border = a.videoItemSelectedBorder;
    } else if (m_hovered) {
        bg = a.videoItemHoverBg;
        border = a.videoItemHoverBorder;
    } else {
        bg = a.videoItemBg;
        border = a.videoItemBorder;
    }

    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(r, 8, 8);

    const int thumbLeft = 14;
    const int thumbTop = (height() - 66) / 2;
    const QRectF thumbRect(thumbLeft, thumbTop, 116, 66);

    if (!m_thumbnail.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(thumbRect, 6, 6);
        p.setClipPath(clip);

        QPixmap scaled = m_thumbnail.scaled(116, 66, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int sx = (116 - scaled.width()) / 2;
        int sy = (66 - scaled.height()) / 2;
        p.drawPixmap(QPointF(thumbRect.left() + sx, thumbRect.top() + sy), scaled);
        p.setClipping(false);

        if (m_hovered) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 100));
            p.drawRoundedRect(thumbRect, 6, 6);

            QPainterPath play;
            qreal cx = thumbRect.center().x();
            qreal cy = thumbRect.center().y();
            play.moveTo(cx - 7, cy - 9);
            play.lineTo(cx - 7, cy + 9);
            play.lineTo(cx + 10, cy);
            play.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::white);
            p.drawPath(play);
        }
    } else {
        QPainterPath clip;
        clip.addRoundedRect(thumbRect, 6, 6);
        p.setClipPath(clip);

        const QColor base = m_darkMode ? QColor(22, 29, 40) : QColor(232, 239, 249);
        const QColor stroke = m_darkMode ? QColor(66, 80, 104) : QColor(198, 211, 230);
        p.setPen(QPen(stroke, 1));
        p.setBrush(base);
        p.drawRoundedRect(thumbRect.adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);

        const QColor icon = m_darkMode ? QColor(132, 154, 184) : QColor(116, 136, 166);
        const QColor fill = m_darkMode ? QColor(31, 41, 58) : QColor(218, 228, 242);
        QRectF markRect = thumbRect.adjusted(26, 10, -26, -10);
        p.setPen(QPen(icon, 1.2));
        p.setBrush(fill);
        p.drawEllipse(markRect);

        QPainterPath playIcon;
        playIcon.moveTo(markRect.center().x() - 4, markRect.center().y() - 7);
        playIcon.lineTo(markRect.center().x() - 4, markRect.center().y() + 7);
        playIcon.lineTo(markRect.center().x() + 8, markRect.center().y());
        playIcon.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(icon);
        p.drawPath(playIcon);
        p.setClipping(false);
    }

    const int textLeft = thumbLeft + 116 + 14;
    const int badgeW = 50;
    const int textRight = width() - 14 - badgeW - 8;

    QFont titleFont = p.font();
    titleFont.setPixelSize(15);
    titleFont.setBold(true);
    p.setFont(titleFont);
    QColor titleColor = m_selected ? a.videoItemSelectedTitle : a.videoItemTitleText;
    QRectF titleRect(textLeft, 16, textRight - textLeft, 24);
    paint::drawElidedText(p, titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_title, titleColor);

    QFont metaFont = p.font();
    metaFont.setPixelSize(13);
    metaFont.setBold(false);
    p.setFont(metaFont);
    QColor metaColor = m_selected ? a.videoItemSelectedMeta : a.videoItemMetaText;
    QRectF metaRect(textLeft, 48, textRight - textLeft, 22);
    paint::drawElidedText(p, metaRect, Qt::AlignLeft | Qt::AlignVCenter, m_meta, metaColor);

    QFontMetrics bfm(metaFont);
    int bw = qMax(badgeW, bfm.horizontalAdvance(m_badgeText) + 18);
    QRectF badgeRect(width() - 14 - bw, 16, bw, 26);
    QColor badgeBg = m_selected ? a.videoBadgeSelectedBg : a.videoBadgeBg;
    QColor badgeBorder = m_selected ? a.videoBadgeSelectedBorder : a.videoBadgeBorder;
    QColor badgeText = m_selected ? a.videoBadgeSelectedText : a.videoBadgeText;
    p.setPen(QPen(badgeBorder, 1));
    p.setBrush(badgeBg);
    p.drawRoundedRect(badgeRect.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    QFont badgeFont = p.font();
    badgeFont.setPixelSize(11);
    badgeFont.setBold(true);
    p.setFont(badgeFont);
    p.setPen(badgeText);
    p.drawText(badgeRect, Qt::AlignCenter, m_badgeText);
}
