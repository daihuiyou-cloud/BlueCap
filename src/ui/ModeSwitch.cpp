#include "ModeSwitch.h"

#include <QButtonGroup>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>

namespace {

QToolButton *createModeButton(const QString &text, bool checked = false)
{
    auto *button = new QToolButton;
    button->setText(text);
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(140, 34);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return button;
}

}

ModeSwitch::ModeSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(42);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(0);

    const QList<QToolButton *> buttons = {
        createModeButton(QStringLiteral("▰  全屏"), true),
        createModeButton(QStringLiteral("▣  区域")),
        createModeButton(QStringLiteral("▭  窗口"))
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

void ModeSwitch::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF pill = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QColor(214, 222, 238));
    painter.setBrush(QColor(247, 250, 255, 224));
    painter.drawRoundedRect(pill, 43, 43);

    painter.setPen(QColor(218, 224, 237));
    const int third = width() / 3;
    painter.drawLine(third, 24, third, height() - 24);
    painter.drawLine(third * 2, 24, third * 2, height() - 24);
}
