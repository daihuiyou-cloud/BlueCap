#include "MainWindow.h"
#include "IconHelper.h"
#include "utils/Theme.h"

#include "RecordPage.h"
#include "RecordingOverlay.h"
#include "SettingsPage.h"
#include "Sidebar.h"
#include "VideoLibraryPage.h"
#include "recorder/RecorderController.h"
#include "storage/VideoLibrary.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>

#include <QGuiApplication>
#include <QResizeEvent>
#include <QScreen>
#include <windows.h>
#include <windowsx.h>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("BlueCap"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app-icon.ico")));
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

    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(this);
    m_overlay = new RecordingOverlay(this);

    setupUI();
    setupTray();
    setupConnections();
    setupHotkey();

    m_shadowDebounce = new QTimer(this);
    m_shadowDebounce->setSingleShot(true);
    m_shadowDebounce->setInterval(100);
    connect(m_shadowDebounce, &QTimer::timeout, this, &MainWindow::renderShadowCache);

    QTimer::singleShot(0, this, &MainWindow::renderShadowCache);
}

void MainWindow::setupUI()
{
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
    m_videoLibraryPage = new VideoLibraryPage(m_library, m_stack);
    m_stack->addWidget(m_videoLibraryPage);
    m_stack->addWidget(new SettingsPage(m_stack));
    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);
}

void MainWindow::setupConnections()
{
    connect(m_sidebar, &Sidebar::pageSelected, m_stack, &QStackedWidget::setCurrentIndex);

    auto *settingsPage = qobject_cast<SettingsPage *>(m_stack->widget(2));
    if (settingsPage) {
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

        connect(settingsPage, &SettingsPage::themeChanged, this, [this](int preference) {
            theme::apply(preference);
            const int resolved = theme::resolve(preference);
            m_darkMode = (resolved == ThemeDark);
            m_recordPage->setDarkMode(m_darkMode);
            m_sidebar->setDarkMode(m_darkMode);
            m_videoLibraryPage->setDarkMode(m_darkMode);
            rebuildPulseStyles();
            if (m_recorder->isRecording())
                m_recordingIndicator->setStyleSheet(m_pulseStyleBright);

            QColor titleNormal = m_darkMode ? QColor(0x9a, 0xa8, 0xbc) : QColor(0x26, 0x33, 0x4b);
            QColor titleActive = m_darkMode ? QColor(0x4d, 0xa3, 0xff) : QColor(0x09, 0x67, 0xf2);
            QColor titleDisabled = m_darkMode ? QColor(0x50, 0x58, 0x68) : QColor(0xa0, 0xaa, 0xb8);

            const QStringList titlePaths = {
                QStringLiteral(":/icons/title-settings.svg"),
                QStringLiteral(":/icons/title-minimize.svg"),
                QStringLiteral(":/icons/title-close.svg")
            };
            QList<QPushButton *> titleBtns = { m_settingsButton, m_minimizeButton, m_closeButton };
            for (int i = 0; i < titleBtns.size() && i < titlePaths.size(); ++i) {
                titleBtns[i]->setIcon(icon::coloredIcon(titlePaths[i], 20, titleNormal, titleActive, titleDisabled));
            }
            m_maximizeButton->setIcon(icon::coloredIcon(
                m_maximized ? QStringLiteral(":/icons/title-restore.svg") : QStringLiteral(":/icons/title-maximize.svg"),
                20, titleNormal, titleActive, titleDisabled));
        });

        settingsPage->loadSettings();
    }

    connect(m_recordPage, &RecordPage::recentVideosClicked, this, [this] {
        m_sidebar->selectPage(1);
        m_stack->setCurrentIndex(1);
    });

    connect(m_recorder, &RecorderController::recordingAreaChanged, this,
        [this](const QRect &area, RecordMode mode) {
            if (mode == RecordMode::FullScreen)
                m_overlay->showForFullscreen();
            else
                m_overlay->showForRegion(area);
        });

    connect(m_recorder, &RecorderController::recordingChanged, this, [this](bool recording) {
        m_recordingIndicator->setVisible(recording);
        if (recording) {
            m_pulseState = false;
            m_recordingIndicator->setStyleSheet(QString());
            m_pulseTimer->start(800);
        } else {
            m_pulseTimer->stop();
            m_recordingIndicator->setStyleSheet(QString());
            m_overlay->hideOverlay();
        }
        m_trayIcon->setIcon(recording ? m_trayIconRecording : m_trayIconIdle);
        m_trayMenu->actions()[1]->setEnabled(!recording);
        m_trayMenu->actions()[2]->setText(recording
            ? QStringLiteral("停止录制")
            : QStringLiteral("开始/停止录制"));
    });

    connect(m_recorder, &RecorderController::errorOccurred, this, [this] {
        m_overlay->hideOverlay();
    });

    connect(m_recordPage, &RecordPage::elapsedUpdated, this, [this](int seconds) {
        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        int s = seconds % 60;
        QString text;
        if (h > 0)
            text = QStringLiteral("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        else
            text = QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        m_overlay->setStatusText(text);
    });
}

void MainWindow::setupHotkey()
{
    qApp->installNativeEventFilter(this);
    m_hotkeyRegistered = RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_SHIFT, 'R');
    if (!m_hotkeyRegistered) {
        qWarning("Failed to register global hotkey (Ctrl+Shift+R). Another application may have claimed it.");
        if (m_trayIcon)
            m_trayIcon->showMessage(QStringLiteral("BlueCap"),
                QStringLiteral("全局快捷键 Ctrl+Shift+R 注册失败，可能被其他程序占用。"),
                QSystemTrayIcon::Warning, 5000);
    }
}

void MainWindow::setupTray()
{
    m_trayIconIdle = makeTrayIcon(false);
    m_trayIconRecording = makeTrayIcon(true);
    m_trayIcon = new QSystemTrayIcon(m_trayIconIdle, this);
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
    rebuildPulseStyles();
    connect(m_pulseTimer, &QTimer::timeout, this, [this] {
        m_pulseState = !m_pulseState;
        m_recordingIndicator->setStyleSheet(m_pulseState
            ? m_pulseStyleBright : m_pulseStyleDim);
    });
}

QIcon MainWindow::makeTrayIcon(bool recording)
{
    QFile file(QStringLiteral(":/icons/app-logo.svg"));
    if (!file.open(QIODevice::ReadOnly))
        return QIcon();

    QString svg = QString::fromUtf8(file.readAll());
    if (recording) {
        svg.replace(QStringLiteral("#80C8FF"), QStringLiteral("#FF8A8A"));
        svg.replace(QStringLiteral("#1776F4"), QStringLiteral("#EF3039"));
        svg.replace(QStringLiteral("#0758E8"), QStringLiteral("#C0202B"));
    }

    QSvgRenderer renderer(svg.toUtf8());
    QIcon icon;
    for (int size : {16, 32, 64}) {
        QPixmap px(size, size);
        px.fill(Qt::transparent);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p, QRectF(0, 0, size, size));
        p.end();
        icon.addPixmap(px);
    }
    return icon;
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
    if (eventType != kMsgGen && eventType != kMsgDsp)
        return false;

    auto *msg = static_cast<MSG *>(message);
    switch (msg->message) {
    case WM_HOTKEY:
        if (msg->wParam != 1)
            return false;
        {
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
        }
        if (result)
            *result = 0;
        return true;

    case WM_NCHITTEST:
        if (IsZoomed(msg->hwnd))
            return false;
        {
            RECT winRect;
            GetWindowRect(msg->hwnd, &winRect);
            int x = GET_X_LPARAM(msg->lParam);
            int y = GET_Y_LPARAM(msg->lParam);

            bool left   = x < winRect.left   + kResizeBorder;
            bool right  = x > winRect.right  - kResizeBorder;
            bool top    = y < winRect.top    + kResizeBorder;
            bool bottom = y > winRect.bottom - kResizeBorder;

            if (top && left)       *result = HTTOPLEFT;
            else if (top && right) *result = HTTOPRIGHT;
            else if (bottom && left) *result = HTBOTTOMLEFT;
            else if (bottom && right) *result = HTBOTTOMRIGHT;
            else if (top)          *result = HTTOP;
            else if (bottom)       *result = HTBOTTOM;
            else if (left)         *result = HTLEFT;
            else if (right)        *result = HTRIGHT;
            else                   return false;
        }
        return true;

    default:
        return false;
    }
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
    painter.drawPixmap(0, 0, m_shadowCache);

    QWidget::paintEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    m_shadowDebounce->start();
    QWidget::resizeEvent(event);
}

void MainWindow::rebuildPulseStyles()
{
    static const char *kBase = "color: #e0525e; font-size: 13px; font-weight: 800; "
        "padding: 4px 12px; border-radius: 12px; ";
    if (m_darkMode) {
        m_pulseStyleBright = QStringLiteral("%1background: rgba(224,82,94,180); border: 1px solid rgba(224,82,94,220);").arg(kBase);
        m_pulseStyleDim = QStringLiteral("%1background: rgba(224,82,94,90); border: 1px solid rgba(224,82,94,150);").arg(kBase);
    } else {
        m_pulseStyleBright = QStringLiteral("%1background: rgba(224,82,94,56); border: 1px solid rgba(224,82,94,128);").arg(kBase);
        m_pulseStyleDim = QStringLiteral("%1background: rgba(224,82,94,30); border: 1px solid rgba(224,82,94,76);").arg(kBase);
    }
}

void MainWindow::renderShadowCache()
{
    if (size().isEmpty()) return;
    m_shadowCache = QPixmap(size());
    m_shadowCache.fill(Qt::transparent);

    QPainter p(&m_shadowCache);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    const QRectF shadowRect = rect().adjusted(12, 12, -12, -12);
    for (int i = 10; i >= 1; --i) {
        p.setBrush(QColor(38, 80, 150, 2 + i));
        p.drawRoundedRect(shadowRect.adjusted(-i, -i + 6, i, i + 6), 34 + i, 34 + i);
    }
}

QWidget *MainWindow::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(56);

    auto *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(24, 0, 20, 0);
    layout->setSpacing(10);

    auto *logo = new QLabel(m_titleBar);
    logo->setObjectName(QStringLiteral("appLogo"));
    logo->setFixedSize(28, 28);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo.svg")).pixmap(28, 28));

    auto *title = new QLabel(QStringLiteral("屏幕录制"), m_titleBar);
    title->setObjectName(QStringLiteral("windowTitle"));

    m_settingsButton = createWindowButton(QString(), QStringLiteral("设置"));
    m_settingsButton->setAccessibleName(QStringLiteral("打开设置页面"));
    m_settingsButton->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/title-settings.svg"), 20,
        QColor(0x26, 0x33, 0x4b), QColor(0x09, 0x67, 0xf2), QColor(0xa0, 0xaa, 0xb8)));
    m_minimizeButton = createWindowButton(QString(), QStringLiteral("最小化"));
    m_minimizeButton->setAccessibleName(QStringLiteral("最小化窗口"));
    m_minimizeButton->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/title-minimize.svg"), 20,
        QColor(0x26, 0x33, 0x4b), QColor(0x09, 0x67, 0xf2), QColor(0xa0, 0xaa, 0xb8)));
    m_maximizeButton = createWindowButton(QString(), QStringLiteral("最大化"));
    m_maximizeButton->setAccessibleName(QStringLiteral("最大化/还原窗口"));
    m_maximizeButton->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/title-maximize.svg"), 20,
        QColor(0x26, 0x33, 0x4b), QColor(0x09, 0x67, 0xf2), QColor(0xa0, 0xaa, 0xb8)));
    m_closeButton = createWindowButton(QString(), QStringLiteral("关闭"), QStringLiteral("closeButton"));
    m_closeButton->setIcon(icon::coloredIcon(
        QStringLiteral(":/icons/title-close.svg"), 20,
        QColor(0x26, 0x33, 0x4b), QColor(0x09, 0x67, 0xf2), QColor(0xa0, 0xaa, 0xb8)));
    m_closeButton->setAccessibleName(QStringLiteral("关闭窗口"));

    layout->addWidget(logo);
    layout->addWidget(title);

    m_recordingIndicator = new QLabel(QStringLiteral("● 录制中"), this);
    m_recordingIndicator->setObjectName(QStringLiteral("recordingIndicator"));
    m_recordingIndicator->setVisible(false);
    layout->addWidget(m_recordingIndicator);
    layout->addStretch();

    layout->addWidget(m_settingsButton);
    layout->addSpacing(10);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_settingsButton, &QPushButton::clicked, this, [this] {
        m_stack->setCurrentIndex(2);
    });
    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_maximizeButton, &QPushButton::clicked, this, [this] {
        if (m_maximized) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return m_titleBar;
}

QPushButton *MainWindow::createTitleBarButton(const QString &text, const QString &tooltip)
{
    auto *button = new QPushButton(text, this);
    button->setObjectName(QStringLiteral("titleButton"));
    button->setFixedSize(30, 30);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tooltip);
    return button;
}

QPushButton *MainWindow::createWindowButton(const QString &iconPath, const QString &tooltip, const QString &objectName)
{
    auto *button = createTitleBarButton(QString(), tooltip);
    if (!objectName.isEmpty()) {
        button->setObjectName(objectName);
    }
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(20, 20));
    return button;
}

void MainWindow::updateMaximizeButton()
{
    m_maximized = isMaximized();
    QColor titleNormal = m_darkMode ? QColor(0x9a, 0xa8, 0xbc) : QColor(0x26, 0x33, 0x4b);
    QColor titleActive = m_darkMode ? QColor(0x4d, 0xa3, 0xff) : QColor(0x09, 0x67, 0xf2);
    QColor titleDisabled = m_darkMode ? QColor(0x50, 0x58, 0x68) : QColor(0xa0, 0xaa, 0xb8);
    m_maximizeButton->setIcon(icon::coloredIcon(
        m_maximized ? QStringLiteral(":/icons/title-restore.svg") : QStringLiteral(":/icons/title-maximize.svg"),
        20, titleNormal, titleActive, titleDisabled));
    m_maximizeButton->setToolTip(m_maximized ? QStringLiteral("还原") : QStringLiteral("最大化"));
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButton();
    }
    QWidget::changeEvent(event);
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    if (!m_titleBar || !m_titleBar->isVisible())
        return false;
    QRect titleRect = m_titleBar->geometry();
    if (!titleRect.contains(position))
        return false;
    if (m_closeButton && m_closeButton->isVisible())
        return position.x() < m_closeButton->geometry().x();
    return true;
}
