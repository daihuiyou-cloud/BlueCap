#include "MainWindow.h"
#include "HotkeyManager.h"
#include "utils/IconHelper.h"
#include "TrayManager.h"
#include "WindowDragHelper.h"
#include "paint/PaintMetrics.h"
#include "theme/Theme.h"
#include "theme/ThemeColors.h"
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
#include <QLabel>
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
    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(paint::Metrics::shellMargin, paint::Metrics::shellMargin,
                               paint::Metrics::shellMargin, paint::Metrics::shellMargin);
    shell->setSpacing(0);

    auto *surface = new SurfaceWidget(this);
    surface->setDarkMode(m_darkMode);

    auto *surfaceLayout = new QVBoxLayout(surface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(0);
    surfaceLayout->addWidget(createTitleBar());

    auto *body = new QWidget(surface);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(20);

    m_sidebar = new Sidebar(body);
    bodyLayout->addWidget(m_sidebar);

    m_stack = new QStackedWidget(body);
    m_recordPage = new RecordPage(m_recorder, m_library, m_stack);
    m_stack->addWidget(m_recordPage);
    m_videoLibraryPage = new VideoLibraryPage(m_library, m_settings, m_stack);
    m_stack->addWidget(m_videoLibraryPage);
    m_settingsPage = new SettingsPage(m_settings, m_stack);
    m_stack->addWidget(m_settingsPage);
    bodyLayout->addWidget(m_stack, 1);

    surfaceLayout->addWidget(body, 1);
    shell->addWidget(surface);
}

void MainWindow::setupPageConnections()
{
    connect(m_sidebar, &Sidebar::pageSelected, m_stack, &QStackedWidget::setCurrentIndex);
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

    connect(settingsPage, &SettingsPage::themeChanged, this, [this](int preference) {
        theme::apply(preference);
        const int resolved = theme::resolve(preference);
        m_darkMode = (resolved == ThemeDark);

    m_recordPage->setDarkMode(m_darkMode);
    m_sidebar->setDarkMode(m_darkMode);
    m_videoLibraryPage->setDarkMode(m_darkMode);
    m_settingsPage->setDarkMode(m_darkMode);

        auto *surface = findChild<SurfaceWidget *>();
        if (surface) surface->setDarkMode(m_darkMode);

        m_recordingIndicator->setDarkMode(m_darkMode);

        for (auto *btn : findChildren<TitleBarButton *>())
            btn->setDarkMode(m_darkMode);

        const auto &tc = ThemeColors::forMode(m_darkMode).titleBar;
        m_maximizeButton->setIcon(icon::coloredIcon(
            m_maximized ? QStringLiteral(":/icons/title-restore.svg") : QStringLiteral(":/icons/title-maximize.svg"),
            20, tc.normal, tc.active, tc.disabled));

        if (m_titleLabel) {
            QPalette tp = m_titleLabel->palette();
            tp.setColor(QPalette::WindowText, ThemeColors::forMode(m_darkMode).app.titleText);
            m_titleLabel->setPalette(tp);
        }
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
    if (msg->message != WM_NCHITTEST)
        return false;

    if (IsZoomed(msg->hwnd))
        return false;

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
    painter.drawPixmap(0, 0, m_shadowCache);

    QWidget::paintEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
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

QWidget *MainWindow::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(paint::Metrics::titleBarHeight);

    auto *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(10);

    auto *logo = new QLabel(m_titleBar);
    logo->setFixedSize(28, 28);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo.png")).pixmap(28, 28));

    m_titleLabel = new QLabel(QStringLiteral("屏幕录制"), m_titleBar);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    {
        QPalette tp = m_titleLabel->palette();
        tp.setColor(QPalette::WindowText, ThemeColors::forMode(false).app.titleText);
        m_titleLabel->setPalette(tp);
    }

    m_settingsButton = new TitleBarButton(QStringLiteral(":/icons/title-settings.svg"),
                                          QStringLiteral("设置"), false, m_titleBar);
    m_settingsButton->setAccessibleName(QStringLiteral("打开设置页面"));

    m_minimizeButton = new TitleBarButton(QStringLiteral(":/icons/title-minimize.svg"),
                                          QStringLiteral("最小化"), false, m_titleBar);
    m_minimizeButton->setAccessibleName(QStringLiteral("最小化窗口"));

    m_maximizeButton = new QPushButton(m_titleBar);
    m_maximizeButton->setFixedSize(30, 30);
    m_maximizeButton->setCursor(Qt::PointingHandCursor);
    m_maximizeButton->setToolTip(QStringLiteral("最大化"));
    m_maximizeButton->setAccessibleName(QStringLiteral("最大化/还原窗口"));
    m_maximizeButton->setFlat(true);
    {
        const auto &tc = ThemeColors::forMode(false).titleBar;
        m_maximizeButton->setIcon(icon::coloredIcon(
            QStringLiteral(":/icons/title-maximize.svg"), 20, tc.normal, tc.active, tc.disabled));
    }

    m_closeButton = new TitleBarButton(QStringLiteral(":/icons/title-close.svg"),
                                       QStringLiteral("关闭"), true, m_titleBar);
    m_closeButton->setAccessibleName(QStringLiteral("关闭窗口"));

    layout->addWidget(logo);
    layout->addWidget(m_titleLabel);

    m_recordingIndicator = new RecordingIndicator(this);
    layout->addWidget(m_recordingIndicator);
    layout->addStretch();

    layout->addWidget(m_settingsButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_settingsButton, &QPushButton::clicked, this, [this] {
        m_stack->setCurrentIndex(2);
    });
    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_maximizeButton, &QPushButton::clicked, this, [this] {
        if (m_maximized)
            showNormal();
        else
            showMaximized();
    });
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return m_titleBar;
}

void MainWindow::updateMaximizeButton()
{
    m_maximized = isMaximized();
    const auto &tc = ThemeColors::forMode(m_darkMode).titleBar;
    m_maximizeButton->setIcon(icon::coloredIcon(
        m_maximized ? QStringLiteral(":/icons/title-restore.svg") : QStringLiteral(":/icons/title-maximize.svg"),
        20, tc.normal, tc.active, tc.disabled));
    m_maximizeButton->setToolTip(m_maximized ? QStringLiteral("还原") : QStringLiteral("最大化"));
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
        updateMaximizeButton();
    QWidget::changeEvent(event);
}

bool MainWindow::inTitleDragArea(const QPoint &position) const
{
    if (!m_titleBar || !m_titleBar->isVisible())
        return false;
    QWidget *parent = m_titleBar->parentWidget();
    if (!parent) return false;
    QPoint mapped = parent->mapFromGlobal(mapToGlobal(position));
    QRect titleRect = m_titleBar->geometry();
    if (!titleRect.contains(mapped))
        return false;
    if (m_closeButton && m_closeButton->isVisible()) {
        QPoint closeBtnTopLeft = parent->mapFromGlobal(
            m_closeButton->mapToGlobal(QPoint(0, 0)));
        return mapped.x() < closeBtnTopLeft.x();
    }
    return true;
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
    if (event->button() == Qt::LeftButton && inTitleDragArea(event->pos())) {
        if (m_maximized)
            showNormal();
        else
            showMaximized();
        event->accept();
        return;
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
