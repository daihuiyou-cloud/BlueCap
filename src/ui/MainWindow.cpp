#include "MainWindow.h"
#include "HotkeyManager.h"
#include "TrayManager.h"
#include "utils/WindowDragHelper.h"
#include "paint/PaintMetrics.h"
#include "style/BlueCapStyle.h"
#include "theme/Theme.h"
#include "widgets/SurfaceWidget.h"
#include "widgets/TitleBarButton.h"
#include "widgets/RecordingIndicator.h"

#include "RecordPage.h"
#include "RecordingOverlay.h"
#include "SettingsPage.h"
#include "Sidebar.h"
#include "VideoLibraryPage.h"
#include "recorder/IRecorderService.h"
#include "recorder/RecorderController.h"
#include "storage/IVideoLibrary.h"
#include "storage/QSettingsRepository.h"
#include "storage/VideoLibrary.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
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
    setMinimumSize(paint::Metrics::windowMinWidth, paint::Metrics::windowMinHeight);

    QSettings geo;
    if (geo.contains(QStringLiteral("window/geometry"))) {
        restoreGeometry(geo.value(QStringLiteral("window/geometry")).toByteArray());
        QRect winGeo = frameGeometry();
        bool onScreen = false;
        for (auto *s : QGuiApplication::screens()) {
            if (s->geometry().intersects(winGeo)) {
                onScreen = true;
                break;
            }
        }
        if (!onScreen) {
            QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
            resize(960, 600);
            move((screen.width() - width()) / 2, (screen.height() - height()) / 2);
        }
    } else {
        QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        move((screen.width() - width()) / 2, (screen.height() - height()) / 2);
    }

    m_settings = new QSettingsRepository(
        QStringLiteral("BlueCap"), QStringLiteral("BlueCap"));
    m_recorder = new RecorderController(this);
    m_library = new VideoLibrary(m_settings, this);
    m_overlay = new RecordingOverlay(this);

    m_tray = new TrayManager(this);
    m_hotkey = new HotkeyManager(this);
    qApp->installNativeEventFilter(this);

    setupUI();
    m_nav = new NavigationController(m_stack, this);
    setupPageConnections();
    setupSettingsConnections();
    setupRecordConnections();
    setupToggleRecord();
    setupTrayConnections();
    m_tray->show();

    if (!m_hotkey->isRegistered()) {
        qWarning("Failed to register global hotkey (Ctrl+Shift+R). Another application may have claimed it.");
        m_tray->showMessage(QStringLiteral("BlueCap"),
            QStringLiteral("全局快捷键 Ctrl+Shift+R 注册失败，可能被其他程序占用。"),
            QSystemTrayIcon::Warning, 5000);
    }

    m_shadowDebounce = new QTimer(this);
    m_shadowDebounce->setSingleShot(true);
    m_shadowDebounce->setInterval(100);
    connect(m_shadowDebounce, &QTimer::timeout, this, &MainWindow::renderShadowCache);

    QTimer::singleShot(0, this, &MainWindow::renderShadowCache);
}

MainWindow::~MainWindow()
{
    qApp->removeNativeEventFilter(this);
}

void MainWindow::setupUI()
{
    m_shell = new QVBoxLayout(this);
    m_shell->setContentsMargins(paint::Metrics::shellMargin, paint::Metrics::shellMargin,
                                paint::Metrics::shellMargin, paint::Metrics::shellMargin);
    m_shell->setSpacing(0);

    auto *surface = new SurfaceWidget(this);
    surface->setDarkMode(ThemeManager::instance().isDark());

    auto *surfaceLayout = new QHBoxLayout(surface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(20);

    m_sidebar = new Sidebar(surface);
    surfaceLayout->addWidget(m_sidebar);

    auto *content = new QWidget(surface);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(createTitleBar());

    m_stack = new QStackedWidget(content);
    m_recordPage = new RecordPage(m_recorder, m_library, m_stack);
    m_stack->addWidget(m_recordPage);
    m_videoLibraryPage = new VideoLibraryPage(m_library, m_settings, m_stack);
    m_stack->addWidget(m_videoLibraryPage);
    m_settingsPage = new SettingsPage(m_settings, m_stack);
    m_stack->addWidget(m_settingsPage);
    contentLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(content, 1);
    m_shell->addWidget(surface);
}

void MainWindow::setupPageConnections()
{
    connect(m_sidebar, &Sidebar::pageSelected, m_nav, &NavigationController::navigate);
}

void MainWindow::setupSettingsConnections()
{
    if (!m_settingsPage)
        return;

    SettingsPage *settingsPage = m_settingsPage;
    connect(settingsPage, &SettingsPage::frameRateChanged,
            m_recorder, &IRecorderService::setFrameRate);
    connect(settingsPage, &SettingsPage::presetChanged,
            m_recorder, &IRecorderService::setPreset);
    connect(settingsPage, &SettingsPage::startTimeoutChanged,
            m_recorder, &IRecorderService::setStartTimeout);
    connect(settingsPage, &SettingsPage::stopTimeoutChanged,
            m_recorder, &IRecorderService::setStopTimeout);
    connect(settingsPage, &SettingsPage::savePathChanged,
            m_recorder, &IRecorderService::setSavePath);
    connect(settingsPage, &SettingsPage::confirmStopChanged,
            m_recordPage, &RecordPage::setConfirmStop);
    connect(settingsPage, &SettingsPage::showCursorChanged,
            m_recorder, &IRecorderService::setShowCursor);

    auto *surface = findChild<SurfaceWidget *>();
    auto &tm = ThemeManager::instance();
    tm.registerUpdater(m_recordPage,           [this](bool dark){ m_recordPage->setDarkMode(dark); });
    tm.registerUpdater(m_sidebar,              [this](bool dark){ m_sidebar->setDarkMode(dark); });
    tm.registerUpdater(m_videoLibraryPage,     [this](bool dark){ m_videoLibraryPage->setDarkMode(dark); });
    tm.registerUpdater(m_settingsPage,         [this](bool dark){ m_settingsPage->setDarkMode(dark); });
    tm.registerUpdater(m_recordingIndicator,   [this](bool dark){ m_recordingIndicator->setDarkMode(dark); });
    tm.registerUpdater(m_minimizeButton,       [this](bool dark){ m_minimizeButton->setDarkMode(dark); });
    tm.registerUpdater(m_closeButton,          [this](bool dark){ m_closeButton->setDarkMode(dark); });
    if (surface)
        tm.registerUpdater(surface,            [surface](bool dark){ surface->setDarkMode(dark); });
    tm.registerUpdater(this, [this](bool) {
        if (!isMaximized())
            renderShadowCache();
        update();
    });

    connect(settingsPage, &SettingsPage::themeChanged, this, [this](int preference) {
        BlueCapStyle::applyTheme(preference);
        ThemeManager::instance().setDarkMode(theme::resolve(preference) == ThemeDark);
    });

    settingsPage->loadSettings();
}

void MainWindow::setupRecordConnections()
{
    connect(m_recorder, &IRecorderService::recordingAreaChanged, this,
        [this](const QRect &area, RecordMode mode) {
            Q_UNUSED(mode);
            m_overlay->showForRegion(area);
        });

    connect(m_recorder, &IRecorderService::recordingChanged, this, [this](bool recording) {
        m_recordingIndicator->setVisible(recording);
        m_hotkey->setRecordingHotkeysEnabled(recording);
        if (recording) {
            m_recordingIndicator->startPulse();
            m_overlay->setHintText(QStringLiteral("Esc 停止录制"));
        } else {
            m_recordingIndicator->stopPulse();
            m_overlay->hideOverlay();
        }
        m_tray->updateRecordingState(recording);
    });

    connect(m_recorder, &IRecorderService::errorOccurred, this, [this] {
        m_overlay->hideOverlay();
    });

    connect(m_recorder, &IRecorderService::recordingWarning, this, [this](const QString &msg) {
        m_tray->showMessage(QStringLiteral("BlueCap"), msg,
            QSystemTrayIcon::Warning, 4000);
    });

    connect(m_recordPage, &RecordPage::requestWindowHide, this, &QWidget::hide);
    connect(m_recordPage, &RecordPage::requestWindowShow, this, [this] { showNormal(); activateWindow(); });

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

void MainWindow::setupToggleRecord()
{
    auto toggleRecord = [this] {
        bool wasRecording = m_recorder->isRecording();
        m_recordPage->toggleRecording();
        bool nowRecording = m_recorder->isRecording();
        if (wasRecording != nowRecording) {
            m_hotkey->setRecordingHotkeysEnabled(nowRecording);
            m_tray->showMessage(QStringLiteral("BlueCap"),
                nowRecording ? QStringLiteral("录制已开始") : QStringLiteral("录制已停止"),
                QSystemTrayIcon::Information, 2500);
            if (nowRecording && !isVisible()) {
                showNormal();
                activateWindow();
            }
        }
    };

    connect(m_hotkey, &HotkeyManager::hotkeyPressed, this, toggleRecord);
    connect(m_hotkey, &HotkeyManager::escapePressed, this, [this] {
        if (m_recorder->isRecording())
            m_recordPage->toggleRecording();
    });
}

void MainWindow::setupTrayConnections()
{
    connect(m_tray, &TrayManager::showWindowRequested, this, [this] {
        if (isVisible() && !isMinimized())
            hide();
        else {
            showNormal();
            activateWindow();
        }
    });

    connect(m_tray, &TrayManager::quickRecordingRequested, this, [this] {
        hide();
        m_recordPage->startQuickRecording();
    });

    connect(m_tray, &TrayManager::toggleRecordingRequested,
            m_recordPage, &RecordPage::toggleRecording);

    connect(m_tray, &TrayManager::quitRequested, qApp, &QApplication::quit);
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    static const QByteArray kMsgGen = QByteArrayLiteral("windows_generic_MSG");
    static const QByteArray kMsgDsp = QByteArrayLiteral("windows_dispatcher_MSG");
    if (eventType != kMsgGen && eventType != kMsgDsp)
        return false;

    auto *msg = static_cast<MSG *>(message);

    if (msg->message == WM_NCCALCSIZE) {
        *result = 0;
        return true;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        auto *mmi = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(MONITORINFO) };
        GetMonitorInfo(monitor, &mi);
        mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
        mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
        mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
        mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
        *result = 0;
        return true;
    }

    if (msg->message != WM_NCHITTEST)
        return false;

    if (IsZoomed(msg->hwnd))
        return false;

    RECT winRect;
    GetWindowRect(msg->hwnd, &winRect);
    int x = GET_X_LPARAM(msg->lParam);
    int y = GET_Y_LPARAM(msg->lParam);

    UINT dpi = GetDpiForWindow(msg->hwnd);
    int border = MulDiv(kResizeBorder, dpi, 96);

    bool left   = x < winRect.left   + border;
    bool right  = x > winRect.right  - border;
    bool top    = y < winRect.top    + border;
    bool bottom = y > winRect.bottom - border;

    if (top && left)       *result = HTTOPLEFT;
    else if (top && right) *result = HTTOPRIGHT;
    else if (bottom && left) *result = HTBOTTOMLEFT;
    else if (bottom && right) *result = HTBOTTOMRIGHT;
    else if (top)          *result = HTTOP;
    else if (bottom)       *result = HTBOTTOM;
    else if (left)         *result = HTLEFT;
    else if (right)        *result = HTRIGHT;
    else                   return false;
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_recorder->isRecording()) {
        hide();
        m_tray->showMessage(QStringLiteral("BlueCap"),
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
    if (!isMaximized())
        painter.drawPixmap(0, 0, m_shadowCache);

    QWidget::paintEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updateWindowState();
    if (!isMaximized())
        m_shadowDebounce->start();
    QWidget::resizeEvent(event);
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
    for (int i = 5; i >= 1; --i) {
        const qreal k = i * 2.0;
        p.setBrush(QColor(38, 80, 150, 2 + i * 3));
        p.drawRoundedRect(shadowRect.adjusted(-k, -k + 4, k, k + 8), 36 + k, 36 + k);
    }
}

void MainWindow::updateWindowState()
{
    bool maximized = isMaximized();
    if (maximized == m_wasMaximized)
        return;
    m_wasMaximized = maximized;
    int margin = maximized ? 0 : paint::Metrics::shellMargin;
    m_shell->setContentsMargins(margin, margin, margin, margin);
    if (auto *surface = findChild<SurfaceWidget *>())
        surface->setRoundedCorners(!maximized);
    if (!maximized)
        renderShadowCache();
    update();
}

QWidget *MainWindow::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(paint::Metrics::titleBarHeight);

    auto *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(0, 0, 20, 0);
    layout->setSpacing(12);

    m_minimizeButton = new TitleBarButton(QStringLiteral(":/icons/title-minimize.svg"),
                                          QStringLiteral("最小化"), false, m_titleBar);
    m_minimizeButton->setAccessibleName(QStringLiteral("最小化窗口"));

    m_closeButton = new TitleBarButton(QStringLiteral(":/icons/title-close.svg"),
                                       QStringLiteral("关闭"), true, m_titleBar);
    m_closeButton->setAccessibleName(QStringLiteral("关闭窗口"));

    m_recordingIndicator = new RecordingIndicator(this);
    layout->addStretch();
    layout->addWidget(m_recordingIndicator);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_closeButton);

    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return m_titleBar;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QPoint mappedPos = m_titleBar && m_titleBar->parentWidget()
        ? m_titleBar->parentWidget()->mapFrom(this, event->pos())
        : event->pos();
    if (window_drag::handlePress(this, m_titleBar, mappedPos, event, m_dragState))
        return;
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_titleBar && m_titleBar->isVisible()) {
        QWidget *parent = m_titleBar->parentWidget();
        QPoint mappedPos = parent ? parent->mapFrom(this, event->pos()) : event->pos();
        if (m_titleBar->geometry().contains(mappedPos)) {
            isMaximized() ? showNormal() : showMaximized();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (window_drag::handleMove(this, event, m_dragState))
        return;
    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    window_drag::handleRelease(event, m_dragState);
    QWidget::mouseReleaseEvent(event);
}
