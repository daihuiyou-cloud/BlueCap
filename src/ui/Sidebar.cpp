#include "Sidebar.h"
#include "paint/PaintMetrics.h"
#include "utils/IconHelper.h"
#include "theme/ThemeColors.h"
#include "widgets/SidebarButton.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(paint::Metrics::sidebarWidth);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 22, 16, 18);
    layout->setSpacing(16);

    auto *brand = new QWidget(this);
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(12);

    auto *logo = new QLabel(brand);
    logo->setFixedSize(46, 46);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo.png")).pixmap(46, 46));

    auto *brandText = new QWidget(brand);
    auto *brandTextLayout = new QVBoxLayout(brandText);
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(3);

    auto *name = new QLabel(QStringLiteral("BlueCap"), brandText);
    QFont nameFont = name->font();
    nameFont.setPixelSize(20);
    nameFont.setBold(true);
    name->setFont(nameFont);

    auto *tag = new QLabel(QStringLiteral("简单 · 高效 · 专业"), brandText);
    QFont tagFont = tag->font();
    tagFont.setPixelSize(10);
    tag->setFont(tagFont);

    brandTextLayout->addWidget(name);
    brandTextLayout->addWidget(tag);
    brandLayout->addWidget(logo);
    brandLayout->addWidget(brandText, 1);
    layout->addWidget(brand);
    layout->addSpacing(12);

    const QStringList paths = {
        QStringLiteral(":/icons/nav-record.svg"),
        QStringLiteral(":/icons/nav-library.svg"),
        QStringLiteral(":/icons/nav-settings.svg")
    };
    const QStringList texts = {
        QStringLiteral("录制"),
        QStringLiteral("视频库"),
        QStringLiteral("设置")
    };
    const QStringList tooltips = {
        QStringLiteral("录制 (Ctrl+Shift+R)"),
        QStringLiteral("视频库"),
        QStringLiteral("设置")
    };

    for (int i = 0; i < paths.size(); ++i) {
        auto *button = new SidebarButton(texts[i], i == 0);
        button->setToolTip(tooltips[i]);
        button->setAccessibleName(tooltips[i]);
        m_group->addButton(button, i);
        layout->addWidget(button);
        m_buttons.append(button);
    }
    layout->addStretch();

    connect(m_group, qOverload<int>(&QButtonGroup::buttonClicked), this,
        [this](int index) { emit pageSelected(static_cast<Page>(index)); });

    updateIcons();
}

void Sidebar::selectPage(Page page)
{
    int idx = static_cast<int>(page);
    auto *button = m_group->button(idx);
    if (button) {
        button->setChecked(true);
        emit pageSelected(page);
    }
}

void Sidebar::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto &a = ThemeColors::forMode(dark).app;
    const auto labels = findChildren<QLabel *>();
    for (auto *label : labels) {
        QPalette p = label->palette();
        p.setColor(QPalette::WindowText,
                   label->font().bold() ? a.titleText : a.recordStatusText);
        label->setPalette(p);
    }
    for (auto *btn : m_buttons) {
        if (auto *sb = qobject_cast<SidebarButton *>(btn))
            sb->setDarkMode(dark);
    }
    updateIcons();
}

void Sidebar::updateIcons()
{
    const auto &sc = ThemeColors::forMode(m_darkMode).sidebar;
    const QStringList paths = {
        QStringLiteral(":/icons/nav-record.svg"),
        QStringLiteral(":/icons/nav-library.svg"),
        QStringLiteral(":/icons/nav-settings.svg")
    };
    for (int i = 0; i < m_buttons.size() && i < paths.size(); ++i) {
        m_buttons[i]->setIcon(icon::coloredIcon(paths[i], 24, sc.normal, sc.active, sc.disabled));
    }
}
