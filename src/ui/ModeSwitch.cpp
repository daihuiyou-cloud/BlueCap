#include "ModeSwitch.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QToolButton>

namespace {

QToolButton *createModeButton(const QString &text, const QString &iconPath, bool checked = false)
{
    auto *button = new QToolButton;
    button->setObjectName(QStringLiteral("modeButton"));
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(24, 24));
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(180, 44);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    return button;
}

}

ModeSwitch::ModeSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(52);
    setMinimumWidth(580);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 5, 6, 5);
    layout->setSpacing(0);

    const QList<QToolButton *> buttons = {
        createModeButton(QStringLiteral("全屏"), QStringLiteral(":/icons/mode-fullscreen.svg"), true),
        createModeButton(QStringLiteral("区域"), QStringLiteral(":/icons/mode-region.svg")),
        createModeButton(QStringLiteral("窗口"), QStringLiteral(":/icons/mode-window.svg"))
    };

    for (int i = 0; i < buttons.size(); ++i) {
        m_group->addButton(buttons[i], i);
        layout->addWidget(buttons[i]);
    }

    connect(m_group, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *button) {
                emit modeChanged(static_cast<RecordMode>(m_group->id(button)));
            });
}

RecordMode ModeSwitch::currentMode() const
{
    if (auto *button = m_group->checkedButton()) {
        return static_cast<RecordMode>(m_group->id(button));
    }
    return RecordMode::FullScreen;
}

void ModeSwitch::setModeEnabled(bool enabled)
{
    const auto buttons = m_group->buttons();
    for (auto *btn : buttons) {
        btn->setEnabled(enabled);
    }
}

void ModeSwitch::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF pill = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QColor(214, 222, 238));
    painter.setBrush(QColor(247, 250, 255, 232));
    painter.drawRoundedRect(pill, 38, 38);

    painter.setPen(QColor(218, 224, 237));
    const int third = width() / 3;
    painter.drawLine(third, 22, third, height() - 22);
    painter.drawLine(third * 2, 22, third * 2, height() - 22);
}
