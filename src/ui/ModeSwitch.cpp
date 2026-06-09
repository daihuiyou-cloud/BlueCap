#include "ModeSwitch.h"
#include "paint/PaintMetrics.h"

#include "widgets/RecordModeCard.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPainter>

ModeSwitch::ModeSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(paint::Metrics::modeCardHeight);
    setMinimumWidth(660);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    const QStringList paths = {
        QStringLiteral(":/icons/mode-fullscreen.svg"),
        QStringLiteral(":/icons/mode-region.svg"),
        QStringLiteral(":/icons/mode-window.svg")
    };
    const QStringList titles = {
        QStringLiteral("全屏录制"),
        QStringLiteral("区域录制"),
        QStringLiteral("窗口录制")
    };
    const QStringList subtitles = {
        QStringLiteral("录制整个屏幕"),
        QStringLiteral("自定义录制区域"),
        QStringLiteral("录制指定窗口")
    };
    const QStringList tooltips = {
        QStringLiteral("全屏：录制整个屏幕"),
        QStringLiteral("区域：拖动选择特定区域进行录制"),
        QStringLiteral("窗口：选择特定窗口进行录制")
    };

    for (int i = 0; i < paths.size(); ++i) {
        auto *button = new RecordModeCard(static_cast<RecordMode>(i), titles[i], subtitles[i], paths[i], this);
        button->setChecked(i == 0);
        button->setToolTip(tooltips[i]);
        button->setAccessibleName(tooltips[i]);
        m_group->addButton(button, i);
        layout->addWidget(button, 1);
        m_buttons.append(button);
    }

    connect(m_group, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *button) {
                emit modeChanged(static_cast<RecordMode>(m_group->id(button)));
            });

    connect(m_group, qOverload<QAbstractButton *, bool>(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *, bool) {
                updateCards();
            });

    updateCards();
}

RecordMode ModeSwitch::currentMode() const
{
    if (auto *button = m_group->checkedButton())
        return static_cast<RecordMode>(m_group->id(button));
    return RecordMode::FullScreen;
}

void ModeSwitch::setDarkMode(bool dark)
{
    m_darkMode = dark;
    updateCards();
}

void ModeSwitch::setModeEnabled(bool enabled)
{
    const auto buttons = m_group->buttons();
    for (auto *btn : buttons)
        btn->setEnabled(enabled);
}

void ModeSwitch::updateCards()
{
    for (auto *button : m_buttons)
        button->setDarkMode(m_darkMode);
    update();
}
