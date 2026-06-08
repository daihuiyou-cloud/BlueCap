#include "ModeSwitch.h"

#include <QButtonGroup>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>
#include <QVariant>

namespace {

QToolButton *createModeButton(const QString &text, const QString &mode, bool checked = false)
{
    auto *button = new QToolButton;
    button->setText(text);
    button->setProperty("mode", mode);
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(250, 74);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return button;
}

}

ModeSwitch::ModeSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(86);
    setMinimumWidth(850);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(0);

    const QList<QToolButton *> buttons = {
        createModeButton(QStringLiteral("▰  全屏"), QStringLiteral("fullscreen"), true),
        createModeButton(QStringLiteral("▣  区域"), QStringLiteral("region")),
        createModeButton(QStringLiteral("▭  窗口"), QStringLiteral("window"))
    };

    for (int i = 0; i < buttons.size(); ++i) {
        m_group->addButton(buttons[i], i);
        layout->addWidget(buttons[i]);
    }

    setStyleSheet(QStringLiteral(R"(
        QToolButton {
            border: 0;
            border-radius: 35px;
            color: #1f2940;
            font-size: 24px;
            font-weight: 700;
            background: transparent;
        }
        QToolButton:checked {
            color: #0967f2;
            background: rgba(255, 255, 255, 0.78);
        }
        QToolButton:hover {
            background: rgba(255, 255, 255, 0.46);
        }
    )"));

    connect(m_group, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *button) {
                emit modeChanged(button->property("mode").toString());
            });
}

QString ModeSwitch::currentMode() const
{
    if (auto *button = m_group->checkedButton()) {
        return button->property("mode").toString();
    }
    return QStringLiteral("fullscreen");
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
