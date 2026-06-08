#pragma once

#include "ModeSwitch.h"

#include <QWidget>

class QElapsedTimer;
class QFrame;
class QLabel;
class QProgressBar;
class QTimer;
class ModeSwitch;
class RecordButton;
class RecorderController;
class VideoLibrary;

class RecordPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordPage(RecorderController *recorder, VideoLibrary *library, QWidget *parent = nullptr);

signals:
    void recentVideosClicked();

public:
    void startQuickRecording();
    void toggleRecording();
    void setConfirmStop(bool confirm);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void handleRecordingChanged(bool recording);
    void handleVideoSaved(const QString &path);
    void handleError(const QString &message);
    void updateRecentVideos(const QStringList &videos);
    void updateElapsedTime();
    void doStartRecording();

private:
    void startRegionSelection();
    void pickWindow();
    void updateStatusForMode(RecordMode mode);

    ModeSwitch *m_modeSwitch = nullptr;
    RecordButton *m_recordButton = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_countdownLabel = nullptr;
    QLabel *m_hotkeyLabel = nullptr;
    QLabel *m_stopStatusLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_stopProgress = nullptr;
    QLabel *m_recentDetailLabel = nullptr;
    QLabel *m_openFolderIcon = nullptr;
    QFrame *m_bottomNavSection = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    QTimer *m_recordingTimer = nullptr;
    QElapsedTimer *m_elapsed = nullptr;
    QTimer *m_countdownTimer = nullptr;
    int m_countdownValue = 0;
    bool m_confirmStop = false;
    QString m_lastSavedPath;
};
