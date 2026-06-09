#include "ModeSwitch.h"
#include "IconHelper.h"
#include "utils/ThemeColors.h"

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
    button->setIconSize(QSize(22, 22));
    button->setCheckable(true);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumSize(168, 42);
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
    setFixedHeight(54);
    setMinimumWidth(560);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
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
    m_darkMode = dark;
    if (dark) {
        m_pillBorder = QColor(72, 84, 106);
        m_pillFill = QColor(31, 38, 50, 222);
        m_dividerColor = QColor(74, 86, 108);
    } else {
        m_pillBorder = QColor(202, 212, 230);
        m_pillFill = QColor(249, 252, 255, 232);
        m_dividerColor = QColor(210, 218, 232);
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
    const auto &mc = ThemeColors::forMode(m_darkMode).modeSwitch;
    const QStringList paths = {
        QStringLiteral(":/icons/mode-fullscreen.svg"),
        QStringLiteral(":/icons/mode-region.svg"),
        QStringLiteral(":/icons/mode-window.svg")
    };
    for (int i = 0; i < m_buttons.size() && i < paths.size(); ++i) {
        m_buttons[i]->setIcon(icon::coloredIcon(paths[i], 24, mc.normal, mc.active, mc.disabled));
    }
}

void ModeSwitch::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF pill = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(m_pillBorder, 1.2));
    painter.setBrush(m_pillFill);
    painter.drawRoundedRect(pill, 38, 38);

    QLinearGradient glow(pill.topLeft(), pill.bottomLeft());
    glow.setColorAt(0.0, m_darkMode ? QColor(255, 255, 255, 18) : QColor(255, 255, 255, 95));
    glow.setColorAt(0.45, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawRoundedRect(pill.adjusted(1, 1, -1, -1), 37, 37);

    painter.setPen(QPen(m_dividerColor, 1));
    const auto buttons = m_group->buttons();
    const int count = buttons.size();
    for (int i = 1; i < count; ++i) {
        const int x = buttons[i]->pos().x();
        painter.drawLine(x, 22, x, height() - 22);
    }
}
