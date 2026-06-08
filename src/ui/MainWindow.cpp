#include "MainWindow.h"

#include "PlaceholderPage.h"
#include "RecordPage.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QButtonGroup>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("BlueCap"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(640, 500);
    setMinimumSize(540, 420);

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);

    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(12, 12, 12, 12);
    shell->setSpacing(0);

    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("surface"));
    surface->setAttribute(Qt::WA_StyledBackground, true);

    auto *shadow = new QGraphicsDropShadowEffect(surface);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(42, 80, 150, 70));
    surface->setGraphicsEffect(shadow);

    auto *surfaceLayout = new QVBoxLayout(surface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(0);

    surfaceLayout->addWidget(createTitleBar());
    surfaceLayout->addWidget(createNavBar());

    auto *body = new QWidget(surface);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_stack = new QStackedWidget(body);
    m_stack->addWidget(new RecordPage(m_recorder, m_library, m_stack));
    m_stack->addWidget(new PlaceholderPage(QStringLiteral("视频库"),
        QStringLiteral("这里会展示最近录制、打开文件位置和基础管理操作。"), m_stack));
    m_stack->addWidget(new PlaceholderPage(QStringLiteral("设置"),
        QStringLiteral("保存路径、画质、帧率、快捷键和音频输入会放在这里。"), m_stack));
    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && inTitleDragArea(event->pos())) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

QWidget *MainWindow::createTitleBar()
{
    auto *titleBar = new QWidget(this);
    titleBar->setObjectName(QStringLiteral("titleBar"));
    titleBar->setFixedHeight(36);

    auto *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(16, 0, 12, 0);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("BlueCap"), titleBar);
    title->setObjectName(QStringLiteral("windowTitle"));

    auto *settingsButton = createWindowButton(QStringLiteral("⚙"), QStringLiteral("设置"));
    auto *minimizeButton = createWindowButton(QStringLiteral("—"), QStringLiteral("最小化"));
    auto *closeButton = createWindowButton(QStringLiteral("×"), QStringLiteral("关闭"));

    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(settingsButton);
    layout->addSpacing(6);
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);

    connect(settingsButton, &QPushButton::clicked, this, [this] {
        m_stack->setCurrentIndex(2);
    });
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return titleBar;
}

QWidget *MainWindow::createNavBar()
{
    auto *navBar = new QWidget(this);
    navBar->setObjectName(QStringLiteral("navBar"));
    navBar->setFixedHeight(36);

    m_navGroup = new QButtonGroup(navBar);
    m_navGroup->setExclusive(true);

    auto *layout = new QHBoxLayout(navBar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const QStringList labels = {
        QStringLiteral("录制"),
        QStringLiteral("视频库"),
        QStringLiteral("设置")
    };

    for (int i = 0; i < labels.size(); ++i) {
        auto *btn = new QPushButton(labels[i], navBar);
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(100, 30);
        m_navGroup->addButton(btn, i);
        layout->addWidget(btn);
    }

    layout->addStretch();

    connect(m_navGroup, qOverload<int>(&QButtonGroup::buttonClicked),
            m_stack, &QStackedWidget::setCurrentIndex);

    return navBar;
}

QPushButton *MainWindow::createWindowButton(const QString &text, const QString &tooltip)
{
    auto *button = new QPushButton(text, this);
    button->setFixedSize(32, 32);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    return position.y() >= 12 && position.y() <= 84
        && position.x() > 12 && position.x() < width() - 140;
}
