#pragma once

#include "NavigationController.h"
#include "theme/ThemeManager.h"
#include "utils/WindowDragHelper.h"

#include <QAbstractNativeEventFilter>
#include <QIcon>
#include <QVBoxLayout>
#include <QWidget>

class QPoint;
class QStackedWidget;
class HotkeyManager;
class IRecorderService;
class ISettingsRepository;
class IVideoLibrary;
class RecordPage;
class RecordingOverlay;
class SettingsPage;
class Sidebar;
class TrayManager;
class VideoLibraryPage;
class TitleBarButton;
class RecordingIndicator;

class MainWindow : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *createTitleBar();
    void updateWindowState();

    void setupUI();
    void setupPageConnections();
    void setupSettingsConnections();
    void setupRecordConnections();
    void setupToggleRecord();
    void setupTrayConnections();

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    NavigationController *m_nav = nullptr;
    ISettingsRepository *m_settings = nullptr;
    IRecorderService *m_recorder = nullptr;
    IVideoLibrary *m_library = nullptr;
    RecordPage *m_recordPage = nullptr;
    VideoLibraryPage *m_videoLibraryPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    RecordingOverlay *m_overlay = nullptr;
    TrayManager *m_tray = nullptr;
    HotkeyManager *m_hotkey = nullptr;

    QWidget *m_titleBar = nullptr;
    TitleBarButton *m_minimizeButton = nullptr;
    TitleBarButton *m_closeButton = nullptr;
    RecordingIndicator *m_recordingIndicator = nullptr;

    window_drag::State m_dragState;

    QVBoxLayout *m_shell = nullptr;
    bool m_wasMaximized = false;

    static constexpr int kResizeBorder = 5;
};
