#include "MainWindow.h"

#include "RecordPage.h"
#include "SettingsPage.h"
#include "VideoLibraryPage.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QApplication>
#include <QButtonGroup>
#include <QHBoxLayout>
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
    resize(640, 500);
    setMinimumSize(540, 420);

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);

    QSettings saved;
    m_recorder->setFrameRate(saved.value(QStringLiteral("settings/frameRate"), 30).toInt());
    m_recorder->setPreset(saved.value(QStringLiteral("settings/preset"), QStringLiteral("ultrafast")).toString());

    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(12, 12, 12, 12);
    shell->setSpacing(0);

    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("surface"));
    surface->setAttribute(Qt::WA_StyledBackground, true);

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

    m_recordPage = new RecordPage(m_recorder, m_library, m_stack);
    m_stack->addWidget(m_recordPage);
    auto *settingsPage = new SettingsPage(m_stack);
    m_stack->addWidget(new VideoLibraryPage(m_library, m_stack));
    m_stack->addWidget(settingsPage);

    connect(settingsPage, &SettingsPage::frameRateChanged,
            m_recorder, &RecorderController::setFrameRate);
    connect(settingsPage, &SettingsPage::presetChanged,
            m_recorder, &RecorderController::setPreset);

    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);

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
            if (result) *result = 0;
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
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    const int m = 12;
    const QRectF sr = rect().adjusted(m, m, -m, -m);

    for (int i = 4; i >= 0; --i) {
        int alpha = 12 - i * 2;
        if (alpha > 0) {
            p.setBrush(QColor(42, 80, 150, alpha));
            p.drawRoundedRect(sr.adjusted(-i, -i + 2, i, i + 2), 20 + i, 20 + i);
        }
    }

    QWidget::paintEvent(event);
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
