#pragma once

#include <QIcon>
#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(QObject *parent = nullptr);
    ~TrayManager() override;

    void show();
    void updateRecordingState(bool recording);
    void showMessage(const QString &title, const QString &message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int duration = 4000);

signals:
    void showWindowRequested();
    void toggleRecordingRequested();
    void quickRecordingRequested();
    void quitRequested();

private:
    QIcon makeTrayIcon(bool recording);

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QIcon m_trayIconIdle;
    QIcon m_trayIconRecording;
    QAction *m_trayQuickAction = nullptr;
    QAction *m_trayRecordAction = nullptr;
};
