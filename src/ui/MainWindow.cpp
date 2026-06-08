#include "MainWindow.h"

#include "RecordPage.h"
#include "SettingsPage.h"
#include "Sidebar.h"
#include "VideoLibraryPage.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("BlueCap"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(1384, 856);
    setMinimumSize(1060, 660);

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);

    QSettings saved;
    m_recorder->setFrameRate(saved.value(QStringLiteral("settings/frameRate"), 30).toInt());
    m_recorder->setPreset(saved.value(QStringLiteral("settings/preset"), QStringLiteral("ultrafast")).toString());

    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(18, 18, 18, 18);
    shell->setSpacing(0);

    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("surface"));
    surface->setAttribute(Qt::WA_StyledBackground, true);

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
    m_recordPage = new RecordPage(m_recorder, m_library, m_stack);
    m_stack->addWidget(m_recordPage);

    auto *settingsPage = new SettingsPage(m_stack);
    m_stack->addWidget(new VideoLibraryPage(m_library, m_stack));
    m_stack->addWidget(settingsPage);
    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);

    connect(m_sidebar, &Sidebar::pageSelected, m_stack, &QStackedWidget::setCurrentIndex);
    connect(settingsPage, &SettingsPage::frameRateChanged,
            m_recorder, &RecorderController::setFrameRate);
    connect(settingsPage, &SettingsPage::presetChanged,
            m_recorder, &RecorderController::setPreset);

    qApp->installNativeEventFilter(this);
}

MainWindow::~MainWindow()
{
    qApp->removeNativeEventFilter(this);
    UnregisterHotKey(nullptr, 1);
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    static const QByteArray kMsgGen = QByteArrayLiteral("windows_generic_MSG");
    static const QByteArray kMsgDsp = QByteArrayLiteral("windows_dispatcher_MSG");
    if (eventType == kMsgGen || eventType == kMsgDsp) {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == 1) {
            m_recordPage->toggleRecording();
            if (result) {
                *result = 0;
            }
            return true;
        }
    }
    return false;
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

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_SHIFT, 'R');
}

void MainWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    UnregisterHotKey(nullptr, 1);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    const QRectF shadowRect = rect().adjusted(18, 18, -18, -18);
    for (int i = 14; i >= 1; --i) {
        painter.setBrush(QColor(38, 80, 150, 2 + i));
        painter.drawRoundedRect(shadowRect.adjusted(-i, -i + 6, i, i + 6), 34 + i, 34 + i);
    }

    QWidget::paintEvent(event);
}

QWidget *MainWindow::createTitleBar()
{
    auto *titleBar = new QWidget(this);
    titleBar->setObjectName(QStringLiteral("titleBar"));
    titleBar->setFixedHeight(128);

    auto *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(54, 0, 48, 0);
    layout->setSpacing(18);

    auto *logo = new QLabel(titleBar);
    logo->setObjectName(QStringLiteral("appLogo"));
    logo->setFixedSize(48, 48);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo.svg")).pixmap(48, 48));

    auto *title = new QLabel(QStringLiteral("屏幕录制"), titleBar);
    title->setObjectName(QStringLiteral("windowTitle"));

    auto *settingsButton = createWindowButton(QStringLiteral(":/icons/title-settings.svg"), QStringLiteral("设置"));
    auto *minimizeButton = createWindowButton(QStringLiteral(":/icons/title-minimize.svg"), QStringLiteral("最小化"));
    auto *closeButton = createWindowButton(QStringLiteral(":/icons/title-close.svg"), QStringLiteral("关闭"), QStringLiteral("closeButton"));

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

QPushButton *MainWindow::createWindowButton(const QString &iconPath, const QString &tooltip, const QString &objectName)
{
    auto *button = new QPushButton(this);
    button->setObjectName(objectName.isEmpty() ? QStringLiteral("titleButton") : objectName);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(32, 32));
    button->setFixedSize(52, 52);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    return position.y() >= 18 && position.y() <= 130
        && position.x() > 18 && position.x() < width() - 230;
}
