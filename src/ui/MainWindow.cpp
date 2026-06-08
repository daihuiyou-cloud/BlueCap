#include "MainWindow.h"

#include "RecordPage.h"
#include "SettingsPage.h"
#include "Sidebar.h"
#include "VideoLibraryPage.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>

#include <QScreen>
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("BlueCap"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(960, 600);
    setMinimumSize(740, 460);

    QSettings geo;
    if (geo.contains(QStringLiteral("window/geometry"))) {
        restoreGeometry(geo.value(QStringLiteral("window/geometry")).toByteArray());
    } else {
        QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        move((screen.width() - width()) / 2, (screen.height() - height()) / 2);
    }

    m_normalWidth = width();
    m_normalHeight = height();

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);

    QSettings saved;
    m_recorder->setFrameRate(saved.value(QStringLiteral("settings/frameRate"), 30).toInt());
    m_recorder->setPreset(saved.value(QStringLiteral("settings/preset"), QStringLiteral("fast")).toString());
    m_recorder->setSavePath(saved.value(QStringLiteral("settings/savePath"), QString()).toString());
    m_recorder->setShowCursor(saved.value(QStringLiteral("settings/showCursor"), true).toBool());

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

    auto *body = new QWidget(surface);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 20, 20);
    bodyLayout->setSpacing(20);

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
    connect(settingsPage, &SettingsPage::startTimeoutChanged,
            m_recorder, &RecorderController::setStartTimeout);
    connect(settingsPage, &SettingsPage::stopTimeoutChanged,
            m_recorder, &RecorderController::setStopTimeout);
    connect(settingsPage, &SettingsPage::savePathChanged,
            m_recorder, &RecorderController::setSavePath);
    connect(settingsPage, &SettingsPage::confirmStopChanged,
            m_recordPage, &RecordPage::setConfirmStop);
    connect(settingsPage, &SettingsPage::showCursorChanged,
            m_recorder, &RecorderController::setShowCursor);

    connect(m_recordPage, &RecordPage::recentVideosClicked, this, [this] {
        m_sidebar->selectPage(1);
        m_stack->setCurrentIndex(1);
    });

    qApp->installNativeEventFilter(this);

    if (!m_hotkeyRegistered) {
        m_hotkeyRegistered = RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_SHIFT, 'R');
    }

    QPixmap normalPx(32, 32);
    normalPx.fill(Qt::transparent);
    { QPainter p(&normalPx); p.setRenderHint(QPainter::Antialiasing);
      p.setBrush(QColor(9, 103, 242)); p.setPen(Qt::NoPen);
      p.drawRoundedRect(2, 2, 28, 28, 6, 6);
      p.setBrush(Qt::white); p.drawEllipse(10, 10, 12, 12); }

    m_trayIcon = new QSystemTrayIcon(QIcon(normalPx), this);
    m_trayMenu = new QMenu(this);
    QAction *showAction = m_trayMenu->addAction(QStringLiteral("显示/隐藏"));
    QAction *quickAction = m_trayMenu->addAction(QStringLiteral("快速全屏录制"));
    QAction *recordAction = m_trayMenu->addAction(QStringLiteral("开始/停止录制"));
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QStringLiteral("退出"));
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(showAction, &QAction::triggered, this, [this] {
        if (isVisible() && !isMinimized()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    });
    connect(quickAction, &QAction::triggered, this, [this] {
        hide();
        m_recordPage->startQuickRecording();
    });
    connect(recordAction, &QAction::triggered, m_recordPage, &RecordPage::toggleRecording);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });

    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, this, [this] {
        m_pulseState = !m_pulseState;
        m_recordingIndicator->setProperty("pulseDim", m_pulseState);
        m_recordingIndicator->style()->unpolish(m_recordingIndicator);
        m_recordingIndicator->style()->polish(m_recordingIndicator);
    });

    auto makeTrayIcon = [this](bool recording) -> QIcon {
        QPixmap px(32, 32);
        px.fill(Qt::transparent);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(recording ? QColor(239, 48, 57) : QColor(9, 103, 242));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(2, 2, 28, 28, 6, 6);
        if (recording) {
            p.fillRect(10, 10, 12, 12, Qt::white);
        } else {
            p.setBrush(Qt::white);
            p.drawEllipse(10, 10, 12, 12);
        }
        p.end();
        return QIcon(px);
    };

    connect(m_recorder, &RecorderController::recordingChanged, this, [this, makeTrayIcon](bool recording) {
        m_recordingIndicator->setVisible(recording);
        if (recording) {
            m_pulseState = false;
            m_recordingIndicator->setProperty("pulseDim", false);
            m_recordingIndicator->style()->unpolish(m_recordingIndicator);
            m_recordingIndicator->style()->polish(m_recordingIndicator);
            m_pulseTimer->start(800);
        } else {
            m_pulseTimer->stop();
            m_recordingIndicator->setProperty("pulseDim", false);
            m_recordingIndicator->style()->unpolish(m_recordingIndicator);
            m_recordingIndicator->style()->polish(m_recordingIndicator);
        }
        m_trayIcon->setIcon(makeTrayIcon(recording));
        m_trayMenu->actions()[1]->setEnabled(!recording);
        m_trayMenu->actions()[2]->setText(recording
            ? QStringLiteral("停止录制")
            : QStringLiteral("开始/停止录制"));
    });
}

MainWindow::~MainWindow()
{
    qApp->removeNativeEventFilter(this);
    if (m_hotkeyRegistered) {
        UnregisterHotKey(nullptr, 1);
    }
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    static const QByteArray kMsgGen = QByteArrayLiteral("windows_generic_MSG");
    static const QByteArray kMsgDsp = QByteArrayLiteral("windows_dispatcher_MSG");
    if (eventType == kMsgGen || eventType == kMsgDsp) {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == 1) {
            bool wasRecording = m_recorder->isRecording();
            m_recordPage->toggleRecording();
            bool nowRecording = m_recorder->isRecording();
            if (wasRecording != nowRecording) {
                m_trayIcon->showMessage(QStringLiteral("BlueCap"),
                    nowRecording ? QStringLiteral("录制已开始")
                                 : QStringLiteral("录制已停止"),
                    QSystemTrayIcon::Information, 2500);
                if (nowRecording && !isVisible()) {
                    showNormal();
                    activateWindow();
                }
            }
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

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && inTitleDragArea(event->pos())) {
        if (m_maximized) {
            showNormal();
        } else {
            showMaximized();
        }
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_recorder->isRecording()) {
        hide();
        m_trayIcon->showMessage(QStringLiteral("BlueCap"),
            QStringLiteral("录制正在进行中，程序已最小化到系统托盘。"),
            QSystemTrayIcon::Information, 3000);
        event->ignore();
    } else {
        QSettings geo;
        geo.setValue(QStringLiteral("window/geometry"), saveGeometry());
        qApp->quit();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    const QRectF shadowRect = rect().adjusted(12, 12, -12, -12);
    for (int i = 10; i >= 1; --i) {
        painter.setBrush(QColor(38, 80, 150, 2 + i));
        painter.drawRoundedRect(shadowRect.adjusted(-i, -i + 6, i, i + 6), 34 + i, 34 + i);
    }

    QWidget::paintEvent(event);
}

QWidget *MainWindow::createTitleBar()
{
    auto *titleBar = new QWidget(this);
    titleBar->setObjectName(QStringLiteral("titleBar"));
    titleBar->setFixedHeight(80);

    auto *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(32, 0, 28, 0);
    layout->setSpacing(14);

    auto *logo = new QLabel(titleBar);
    logo->setObjectName(QStringLiteral("appLogo"));
    logo->setFixedSize(32, 32);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo.svg")).pixmap(32, 32));

    auto *title = new QLabel(QStringLiteral("屏幕录制"), titleBar);
    title->setObjectName(QStringLiteral("windowTitle"));

    auto *settingsButton = createWindowButton(QStringLiteral(":/icons/title-settings.svg"), QStringLiteral("设置"));
    settingsButton->setAccessibleName(QStringLiteral("打开设置页面"));
    auto *minimizeButton = createWindowButton(QStringLiteral(":/icons/title-minimize.svg"), QStringLiteral("最小化"));
    minimizeButton->setAccessibleName(QStringLiteral("最小化窗口"));
    m_maximizeButton = createTitleBarButton(QStringLiteral("□"), QStringLiteral("最大化"));
    m_maximizeButton->setAccessibleName(QStringLiteral("最大化/还原窗口"));
    auto *closeButton = createWindowButton(QStringLiteral(":/icons/title-close.svg"), QStringLiteral("关闭"), QStringLiteral("closeButton"));
    closeButton->setAccessibleName(QStringLiteral("关闭窗口"));

    layout->addWidget(logo);
    layout->addWidget(title);

    m_recordingIndicator = new QLabel(QStringLiteral("● 录制中"), this);
    m_recordingIndicator->setObjectName(QStringLiteral("recordingIndicator"));
    m_recordingIndicator->setVisible(false);
    layout->addWidget(m_recordingIndicator);
    layout->addStretch();

    layout->addWidget(settingsButton);
    layout->addSpacing(18);
    layout->addWidget(minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(closeButton);

    connect(settingsButton, &QPushButton::clicked, this, [this] {
        m_stack->setCurrentIndex(2);
    });
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_maximizeButton, &QPushButton::clicked, this, [this] {
        if (m_maximized) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return titleBar;
}

QPushButton *MainWindow::createWindowButton(const QString &iconPath, const QString &tooltip, const QString &objectName)
{
    auto *button = new QPushButton(this);
    button->setObjectName(objectName.isEmpty() ? QStringLiteral("titleButton") : objectName);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(22, 22));
    button->setFixedSize(36, 36);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

QPushButton *MainWindow::createTitleBarButton(const QString &text, const QString &tooltip)
{
    auto *button = new QPushButton(text, this);
    button->setObjectName(QStringLiteral("titleButton"));
    button->setFixedSize(36, 36);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

void MainWindow::updateMaximizeButton()
{
    m_maximized = isMaximized();
    m_maximizeButton->setText(m_maximized ? QStringLiteral("❐") : QStringLiteral("□"));
    m_maximizeButton->setToolTip(m_maximized ? QStringLiteral("还原") : QStringLiteral("最大化"));
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButton();
    }
    QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!isMaximized() && !isMinimized()) {
        m_normalWidth = event->size().width();
        m_normalHeight = event->size().height();
    }
    QWidget::resizeEvent(event);
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    return position.y() >= 12 && position.y() <= 90
        && position.x() > 12 && position.x() < width() - 200;
}
