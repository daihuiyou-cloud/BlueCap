#include "MainWindow.h"

#include "PlaceholderPage.h"
#include "RecordPage.h"
#include "Sidebar.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

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
    resize(1384, 856);
    setMinimumSize(1040, 680);

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);

    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(16, 16, 16, 16);
    shell->setSpacing(0);

    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("surface"));
    surface->setAttribute(Qt::WA_StyledBackground, true);

    auto *shadow = new QGraphicsDropShadowEffect(surface);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(42, 80, 150, 80));
    surface->setGraphicsEffect(shadow);

    auto *surfaceLayout = new QVBoxLayout(surface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(0);

    surfaceLayout->addWidget(createTitleBar());

    auto *body = new QWidget(surface);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 32, 32);
    bodyLayout->setSpacing(32);

    m_sidebar = new Sidebar(body);
    bodyLayout->addWidget(m_sidebar);

    m_stack = new QStackedWidget(body);
    m_stack->addWidget(new RecordPage(m_recorder, m_library, m_stack));
    m_stack->addWidget(new PlaceholderPage(QStringLiteral("视频库"),
        QStringLiteral("这里会展示最近录制、打开文件位置和基础管理操作。"), m_stack));
    m_stack->addWidget(new PlaceholderPage(QStringLiteral("设置"),
        QStringLiteral("保存路径、画质、帧率、快捷键和音频输入会放在这里。"), m_stack));
    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);

    connect(m_sidebar, &Sidebar::pageSelected, m_stack, &QStackedWidget::setCurrentIndex);
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
    titleBar->setFixedHeight(128);

    auto *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(54, 0, 48, 0);
    layout->setSpacing(20);

    auto *logo = new QLabel(titleBar);
    logo->setFixedSize(50, 50);
    logo->setObjectName(QStringLiteral("logo"));
    logo->setText(QStringLiteral("●"));
    logo->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral("屏幕录制"), titleBar);
    title->setObjectName(QStringLiteral("windowTitle"));

    auto *settingsButton = createWindowButton(QStringLiteral("⚙"), QStringLiteral("设置"));
    auto *minimizeButton = createWindowButton(QStringLiteral("—"), QStringLiteral("最小化"));
    auto *closeButton = createWindowButton(QStringLiteral("×"), QStringLiteral("关闭"));

    layout->addWidget(logo);
    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(settingsButton);
    layout->addSpacing(18);
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);

    connect(settingsButton, &QPushButton::clicked, this, [this] {
        m_stack->setCurrentIndex(2);
    });
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return titleBar;
}

QPushButton *MainWindow::createWindowButton(const QString &text, const QString &tooltip)
{
    auto *button = new QPushButton(text, this);
    button->setFixedSize(52, 52);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    return position.y() >= 16 && position.y() <= 128
        && position.x() > 16 && position.x() < width() - 210;
}
