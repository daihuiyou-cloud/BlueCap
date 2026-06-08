#include "Sidebar.h"

#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *createNavButton(const QString &text, bool checked = false)
{
    auto *button = new QPushButton(text);
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(102);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

}

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(280);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 24, 24, 34);
    layout->setSpacing(34);

    const QList<QPushButton *> buttons = {
        createNavButton(QStringLiteral("▶  录制"), true),
        createNavButton(QStringLiteral("□  视频库")),
        createNavButton(QStringLiteral("◇  设置"))
    };

    for (int i = 0; i < buttons.size(); ++i) {
        m_group->addButton(buttons[i], i);
        layout->addWidget(buttons[i]);
    }
    layout->addStretch();

    setStyleSheet(QStringLiteral(R"(
        QPushButton {
            border: 0;
            border-radius: 28px;
            text-align: left;
            padding-left: 34px;
            color: #172033;
            background: transparent;
            font-size: 25px;
            font-weight: 700;
        }
        QPushButton:checked {
            color: #0967f2;
            background: rgba(222, 237, 255, 0.86);
        }
        QPushButton:hover {
            background: rgba(232, 242, 255, 0.66);
        }
    )"));

    connect(m_group, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &Sidebar::pageSelected);
}
