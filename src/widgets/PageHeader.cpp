#include "PageHeader.h"
#include "theme/ThemeColors.h"

#include <QPainter>

PageHeader::PageHeader(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setObjectName(QStringLiteral("pageHeader"));
    setMinimumHeight(34);
}

void PageHeader::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void PageHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QFont font = painter.font();
    font.setPixelSize(22);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(a.pageHeaderText);
    painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, text());
}
