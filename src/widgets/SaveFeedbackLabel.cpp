#include "SaveFeedbackLabel.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>

SaveFeedbackLabel::SaveFeedbackLabel(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setVisible(false);
}

void SaveFeedbackLabel::setSuccess(bool success)
{
    m_success = success;
    m_hasResult = true;
    update();
}

void SaveFeedbackLabel::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void SaveFeedbackLabel::paintEvent(QPaintEvent *event)
{
    if (!m_hasResult) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QColor textColor = m_success ? a.saveFeedbackSuccessText : a.saveFeedbackFailText;
    QColor bgColor = m_success ? a.saveFeedbackSuccessBg : a.saveFeedbackFailBg;

    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    QFont font = painter.font();
    font.setPixelSize(13);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(textColor);
    painter.drawText(r.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, text());
}
