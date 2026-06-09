#pragma once

#include "RecordMode.h"

#include <QElapsedTimer>
#include <QWidget>
class QComboBox;
class QLabel;
class QProgressBar;
class QScreen;
class QTimer;
class QVBoxLayout;
class IRecorderService;
class ModeSwitch;
class RecordButton;
class RecordPageBottomBar;
class VideoLibrary;

class RecordPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordPage(IRecorderService *recorder, VideoLibrary *library, QWidget *parent = nullptr);

signals:
    void recentVideosClicked();
    void elapsedUpdated(int seconds);

public:
    void startQuickRecording();
    void toggleRecording();
    void setConfirmStop(bool confirm);
    void setDarkMode(bool dark);

public slots:
    void handleRecordingChanged(bool recording);
    void handleVideoSaved(const QString &path);
    void handleError(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateRecentVideos(const QStringList &videos);
    void updateElapsedTime();
    void updateStopProgress();
    void doStartRecording();

private:
    void openSaveFolder();
    void startRegionSelection();
    void pickWindow();
    void updateStatusForMode(RecordMode mode);
    void updateScreenCombo();
    QScreen *selectedScreen() const;

    ModeSwitch *m_modeSwitch = nullptr;
    QComboBox *m_screenCombo = nullptr;
    RecordButton *m_recordButton = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_countdownLabel = nullptr;
    QLabel *m_hotkeyLabel = nullptr;
    QLabel *m_stopStatusLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_stopProgress = nullptr;
    RecordPageBottomBar *m_bottomBar = nullptr;
    IRecorderService *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    QTimer *m_recordingTimer = nullptr;
    QElapsedTimer m_elapsed;
    QTimer *m_countdownTimer = nullptr;
    QTimer *m_stopProgressTimer = nullptr;
    int m_countdownValue = 0;
    bool m_confirmStop = false;
    bool m_darkMode = false;
    bool m_hiddenForRecording = false;
    bool m_statusOpensSavedVideo = false;
    bool m_regionCommitted = false;
    QString m_lastSavedPath;
    QString m_stopOutputPath;
};
