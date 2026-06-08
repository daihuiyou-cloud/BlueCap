#include "ModeSwitch.h"
#include "IconHelper.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>

namespace {

QToolButton *createModeButton(const QString &text, bool checked = false)
{
    auto *button = new QToolButton;
    button->setObjectName(QStringLiteral("modeButton"));
    button->setText(text);
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

    const QStringList paths = {
        QStringLiteral(":/icons/mode-fullscreen.svg"),
        QStringLiteral(":/icons/mode-region.svg"),
        QStringLiteral(":/icons/mode-window.svg")
    };
    const QStringList texts = {
        QStringLiteral("全屏"),
        QStringLiteral("区域"),
        QStringLiteral("窗口")
    };
    const QStringList tooltips = {
        QStringLiteral("全屏：录制整个屏幕"),
        QStringLiteral("区域：拖动选择特定区域进行录制"),
        QStringLiteral("窗口：选择特定窗口进行录制")
    };

    for (int i = 0; i < paths.size(); ++i) {
        auto *button = createModeButton(texts[i], i == 0);
        button->setToolTip(tooltips[i]);
        button->setAccessibleName(tooltips[i]);
        m_group->addButton(button, i);
        layout->addWidget(button);
        m_buttons.append(button);
    }

    connect(m_group, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *button) {
                emit modeChanged(static_cast<RecordMode>(m_group->id(button)));
            });

    updateIcons();
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
    updateIcons();
    update();
}

void ModeSwitch::setModeEnabled(bool enabled)
{
    const auto buttons = m_group->buttons();
    for (auto *btn : buttons) {
        btn->setEnabled(enabled);
    }
}

void ModeSwitch::updateIcons()
{
    const int size = 24;
    QColor normal, active, disabled;
    if (m_darkMode) {
        normal = QColor(0xb0, 0xbc, 0xc8);
        active = QColor(0x4d, 0xa3, 0xff);
        disabled = QColor(0x50, 0x58, 0x68);
    } else {
        normal = QColor(0x1f, 0x29, 0x40);
        active = QColor(0x09, 0x67, 0xf2);
        disabled = QColor(0xa0, 0xaa, 0xb8);
    }

    const QStringList paths = {
        QStringLiteral(":/icons/mode-fullscreen.svg"),
        QStringLiteral(":/icons/mode-region.svg"),
        QStringLiteral(":/icons/mode-window.svg")
    };

    for (int i = 0; i < m_buttons.size() && i < paths.size(); ++i) {
        m_buttons[i]->setIcon(icon::coloredIcon(paths[i], size, normal, active, disabled));
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
    const auto buttons = m_group->buttons();
    const int count = buttons.size();
    for (int i = 1; i < count; ++i) {
        const int x = buttons[i]->pos().x();
        painter.drawLine(x, 22, x, height() - 22);
    }
}
