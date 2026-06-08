#pragma once

#include <QAbstractNativeEventFilter>
#include <QWidget>

class QLabel;
class QMenu;
class QPoint;
class QPushButton;
class QStackedWidget;
class QTimer;
class QSystemTrayIcon;
class RecordPage;
class RecorderController;
class RecordingOverlay;
class Sidebar;
class VideoLibrary;

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
    void setupHotkey();
    void setupTray();
    QIcon makeTrayIcon(bool recording);

    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    RecordPage *m_recordPage = nullptr;
    QLabel *m_recordingIndicator = nullptr;
    QTimer *m_pulseTimer = nullptr;
    bool m_pulseState = false;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QPoint m_dragPosition;
    bool m_dragging = false;
    bool m_maximized = false;
    bool m_hotkeyRegistered = false;
    QPushButton *m_maximizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QWidget *m_titleBar = nullptr;
    RecordingOverlay *m_overlay = nullptr;
    QPixmap m_shadowCache;
    void renderShadowCache();
};
