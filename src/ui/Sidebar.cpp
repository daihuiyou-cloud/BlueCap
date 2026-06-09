#include "Sidebar.h"
#include "IconHelper.h"
#include "utils/ThemeColors.h"

#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *createNavButton(const QString &text, bool checked = false)
{
    auto *button = new QPushButton(text);
    button->setObjectName(QStringLiteral("sidebarButton"));
    button->setIconSize(QSize(24, 24));
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(64);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

}

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(180);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 16, 20);
    layout->setSpacing(20);

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
        QStringLiteral("录制（Ctrl+Shift+R）"),
        QStringLiteral("视频库"),
        QStringLiteral("设置")
    };

    for (int i = 0; i < paths.size(); ++i) {
        auto *button = createNavButton(texts[i], i == 0);
        button->setToolTip(tooltips[i]);
        button->setAccessibleName(tooltips[i]);
        m_group->addButton(button, i);
        layout->addWidget(button);
        m_buttons.append(button);
    }
    layout->addStretch();

    connect(m_group, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &Sidebar::pageSelected);

    updateIcons();
}

void Sidebar::selectPage(int index)
{
    auto *button = m_group->button(index);
    if (button) {
        button->setChecked(true);
        emit pageSelected(index);
    }
}

void Sidebar::setDarkMode(bool dark)
{
    m_darkMode = dark;
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
