#include "RecordPageBottomBar.h"
#include "utils/IconHelper.h"
#include "theme/ThemeColors.h"
#include "widgets/BottomNavSection.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

RecordPageBottomBar::RecordPageBottomBar(QWidget *parent)
    : BottomBar(parent)
{
    setMinimumHeight(74);

    auto *bottomLayout = new QHBoxLayout(this);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    m_bottomNavSection = new BottomNavSection(this);
    m_bottomNavSection->setToolTip(QStringLiteral("点击查看全部录制视频"));
    auto *navLayout = new QHBoxLayout(m_bottomNavSection);
    navLayout->setContentsMargins(30, 0, 20, 0);
    navLayout->setSpacing(15);

    m_recentIcon = new QLabel(m_bottomNavSection);
    m_recentIcon->setPixmap(icon::renderSvg(
        QStringLiteral(":/icons/clock.svg"), ThemeColors::forMode(false).bottomBar.normal, 24));

    m_recentTitle = new QLabel(QStringLiteral("最近视频"), m_bottomNavSection);
    QFont titleFont = m_recentTitle->font();
    titleFont.setPixelSize(16);
    titleFont.setBold(true);
    m_recentTitle->setFont(titleFont);
    m_recentTitle->setPalette([&]() {
        QPalette p; p.setColor(QPalette::WindowText, ThemeColors::forMode(false).app.bottomIconText); return p; }());

    m_recentDetailLabel = new QLabel(m_bottomNavSection);
    m_recentDetailLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QFont detailFont = m_recentDetailLabel->font();
    detailFont.setPixelSize(12);
    m_recentDetailLabel->setFont(detailFont);

    auto *recentTextLayout = new QVBoxLayout;
    recentTextLayout->setContentsMargins(0, 0, 0, 0);
    recentTextLayout->setSpacing(2);
    recentTextLayout->addWidget(m_recentTitle);
    recentTextLayout->addWidget(m_recentDetailLabel);

    navLayout->addWidget(m_recentIcon);
    navLayout->addLayout(recentTextLayout, 1);
    navLayout->addSpacing(4);

    m_separator = new QFrame(this);
    m_separator->setFixedWidth(1);
    m_separator->setFixedHeight(36);

    auto *rightSection = new QWidget(this);
    auto *rightLayout = new QHBoxLayout(rightSection);
    rightLayout->setContentsMargins(18, 0, 26, 0);
    rightLayout->setSpacing(15);

    m_keyboardIcon = new QLabel(rightSection);
    m_keyboardIcon->setPixmap(icon::renderSvg(
        QStringLiteral(":/icons/keyboard.svg"), ThemeColors::forMode(false).bottomBar.normal, 24));

    m_shortcutLabel = new QLabel(QStringLiteral("Ctrl + Shift + R"), rightSection);
    QFont shortcutFont = m_shortcutLabel->font();
    shortcutFont.setPixelSize(16);
    shortcutFont.setBold(true);
    m_shortcutLabel->setFont(shortcutFont);
    m_shortcutLabel->setPalette([&]() {
        QPalette p; p.setColor(QPalette::WindowText, ThemeColors::forMode(false).app.shortcutText); return p; }());

    const auto &bc = ThemeColors::forMode(false).bottomBar;
    m_openFolderIcon = new QPushButton(rightSection);
    m_openFolderIcon->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/folder.svg"), 20, bc.normal, bc.active, bc.disabled));
    m_openFolderIcon->setIconSize(QSize(20, 20));
    m_openFolderIcon->setFixedSize(48, 34);
    m_openFolderIcon->setCursor(Qt::PointingHandCursor);
    m_openFolderIcon->setToolTip(QStringLiteral("打开保存文件夹"));
    m_openFolderIcon->setFlat(true);
    m_openFolderIcon->setProperty("bluecapRole", QStringLiteral("bottomIcon"));

    m_chevronIcon = new QLabel(this);
    m_chevronIcon->setPixmap(icon::renderSvg(
        QStringLiteral(":/icons/chevron-right.svg"), ThemeColors::forMode(false).bottomBar.normal, 20));

    rightLayout->addWidget(m_keyboardIcon);
    rightLayout->addWidget(m_shortcutLabel);
    rightLayout->addWidget(m_openFolderIcon);
    rightLayout->addSpacing(4);
    rightLayout->addWidget(m_chevronIcon);

    bottomLayout->addWidget(m_bottomNavSection, 1);
    bottomLayout->addWidget(m_separator);
    bottomLayout->addWidget(rightSection);

    m_bottomNavSection->installEventFilter(this);

    connect(m_openFolderIcon, &QPushButton::clicked, this, &RecordPageBottomBar::openSaveFolderRequested);
}

void RecordPageBottomBar::setRecentVideoDetail(const QString &text)
{
    m_recentDetailLabel->setText(text);
}

void RecordPageBottomBar::setDarkMode(bool dark)
{
    m_darkMode = dark;
    BottomBar::setDarkMode(dark);
    updateIcons();
    if (m_bottomNavSection)
        m_bottomNavSection->setDarkMode(dark);
    const auto &a = ThemeColors::forMode(dark).app;
    if (m_separator) {
        QPalette sp = m_separator->palette();
        sp.setColor(QPalette::Window, a.bottomSeparator);
        m_separator->setPalette(sp);
        m_separator->setAutoFillBackground(true);
    }
    if (m_recentTitle) {
        QPalette p;
        p.setColor(QPalette::WindowText, a.bottomIconText);
        m_recentTitle->setPalette(p);
    }
    if (m_shortcutLabel) {
        QPalette p;
        p.setColor(QPalette::WindowText, a.shortcutText);
        m_shortcutLabel->setPalette(p);
    }
}

void RecordPageBottomBar::updateIcons()
{
    const auto &bc = ThemeColors::forMode(m_darkMode).bottomBar;
    m_recentIcon->setPixmap(icon::renderSvg(QStringLiteral(":/icons/clock.svg"), bc.normal, 24));
    m_keyboardIcon->setPixmap(icon::renderSvg(QStringLiteral(":/icons/keyboard.svg"), bc.normal, 24));
    m_chevronIcon->setPixmap(icon::renderSvg(QStringLiteral(":/icons/chevron-right.svg"), bc.normal, 20));
    m_openFolderIcon->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/folder.svg"), 20, bc.normal, bc.active, bc.disabled));
}

bool RecordPageBottomBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_bottomNavSection && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            emit recentVideosClicked();
            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}
