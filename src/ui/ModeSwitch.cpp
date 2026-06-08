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
    , m_pillBorder(214, 222, 238)
    , m_pillFill(247, 250, 255, 232)
    , m_dividerColor(218, 224, 237)
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
    const QStringList tooltips = {
        QStringLiteral("全屏：录制整个屏幕"),
        QStringLiteral("区域：拖动选择特定区域进行录制"),
        QStringLiteral("窗口：选择特定窗口进行录制")
    };

    for (int i = 0; i < buttons.size(); ++i) {
        m_group->addButton(buttons[i], i);
        buttons[i]->setToolTip(tooltips[i]);
        buttons[i]->setAccessibleName(tooltips[i]);
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

void ModeSwitch::setDarkMode(bool dark)
{
    if (dark) {
        m_pillBorder = QColor(60, 70, 85);
        m_pillFill = QColor(35, 42, 55, 232);
        m_dividerColor = QColor(60, 70, 85);
    } else {
        m_pillBorder = QColor(214, 222, 238);
        m_pillFill = QColor(247, 250, 255, 232);
        m_dividerColor = QColor(218, 224, 237);
    }
    update();
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
    painter.setPen(m_pillBorder);
    painter.setBrush(m_pillFill);
    painter.drawRoundedRect(pill, 38, 38);

    painter.setPen(m_dividerColor);
    const int count = m_group->buttons().size();
    if (count > 1) {
        const int step = width() / count;
        for (int i = 1; i < count; ++i) {
            painter.drawLine(step * i, 22, step * i, height() - 22);
        }
    }
}
