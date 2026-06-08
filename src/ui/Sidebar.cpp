#include "Sidebar.h"

#include <QButtonGroup>
#include <QIcon>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *createNavButton(const QString &text, const QString &iconPath, bool checked = false)
{
    auto *button = new QPushButton(text);
    button->setObjectName(QStringLiteral("sidebarButton"));
    button->setIcon(QIcon(iconPath));
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

    const QList<QPushButton *> buttons = {
        createNavButton(QStringLiteral("录制(&R)"), QStringLiteral(":/icons/nav-record.svg"), true),
        createNavButton(QStringLiteral("视频库(&L)"), QStringLiteral(":/icons/nav-library.svg")),
        createNavButton(QStringLiteral("设置(&S)"), QStringLiteral(":/icons/nav-settings.svg"))
    };

    for (int i = 0; i < buttons.size(); ++i) {
        m_group->addButton(buttons[i], i);
        layout->addWidget(buttons[i]);
    }
    layout->addStretch();

    connect(m_group, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &Sidebar::pageSelected);
}

void Sidebar::selectPage(int index)
{
    auto *button = m_group->button(index);
    if (button) {
        button->setChecked(true);
        emit pageSelected(index);
    }
}
