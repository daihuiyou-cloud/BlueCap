#pragma once

#include <QAbstractNativeEventFilter>
#include <QWidget>

class QLabel;
class QMenu;
class QPoint;
class QPushButton;
class QStackedWidget;
class QSystemTrayIcon;
class RecordPage;
class RecorderController;
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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QWidget *createTitleBar();
    QPushButton *createWindowButton(const QString &iconPath, const QString &tooltip, const QString &objectName = {});
    bool inTitleDragArea(const QPoint &position) const;

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
    bool m_hotkeyRegistered = false;
};
