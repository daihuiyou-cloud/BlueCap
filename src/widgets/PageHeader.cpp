#include "PageHeader.h"
#include "theme/ThemeColors.h"

PageHeader::PageHeader(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setObjectName(QStringLiteral("pageHeader"));
    setMinimumHeight(34);
    QFont f = font();
    f.setPixelSize(22);
    f.setBold(true);
    setFont(f);
}

void PageHeader::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto &a = ThemeColors::forMode(dark).app;
    QPalette p = palette();
    p.setColor(QPalette::WindowText, a.pageHeaderText);
    setPalette(p);
}
