#pragma once

#include <QAbstractNativeEventFilter>
#include <QIcon>
#include <QWidget>

class QLabel;
class QPoint;
class QPushButton;
class QStackedWidget;
class QTimer;
class HotkeyManager;
class RecordPage;
class RecorderController;
class RecordingOverlay;
class Sidebar;
class TrayManager;
class VideoLibrary;
class VideoLibraryPage;

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
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *createTitleBar();
    QPushButton *createWindowButton(const QString &iconPath, const QString &tooltip, const QString &objectName = {});
    QPushButton *createTitleBarButton(const QString &text, const QString &tooltip);
    void updateMaximizeButton();
    bool inTitleDragArea(const QPoint &position) const;

    void setupUI();
    void setupConnections();
    void setupPulseTimer();
    void renderShadowCache();
    void rebuildPulseStyles();

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    RecordPage *m_recordPage = nullptr;
    VideoLibraryPage *m_videoLibraryPage = nullptr;
    RecordingOverlay *m_overlay = nullptr;
    TrayManager *m_tray = nullptr;
    HotkeyManager *m_hotkey = nullptr;

    QWidget *m_titleBar = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_minimizeButton = nullptr;
    QPushButton *m_maximizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QLabel *m_recordingIndicator = nullptr;

    QPoint m_dragPosition;
    bool m_dragging = false;
    bool m_maximized = false;
    bool m_darkMode = false;

    QPixmap m_shadowCache;
    QTimer *m_shadowDebounce = nullptr;

    QTimer *m_pulseTimer = nullptr;
    bool m_pulseState = false;
    QString m_pulseStyleBright;
    QString m_pulseStyleDim;

    static constexpr int kResizeBorder = 5;
};
