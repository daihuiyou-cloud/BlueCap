#pragma once

#include "RecordMode.h"
#include "paint/PaintTheme.h"

#include <QElapsedTimer>
#include <QWidget>

class QComboBox;
class QLabel;
class QProgressBar;
class QScreen;
class QTimer;
class QVBoxLayout;
class AudioToggleCard;
class IRecorderService;
class IVideoLibrary;
class ModeSwitch;
class RecordButton;

class RecordPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordPage(IRecorderService *recorder, IVideoLibrary *library, QWidget *parent = nullptr);

signals:
    void elapsedUpdated(int seconds);
    void requestWindowHide();
    void requestWindowShow();

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
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateElapsedTime();
    void updateStopProgress();
    void doStartRecording();

private:
    void openSaveFolder();
    void updateLabelColors();
    void startRegionSelection();
    void pickWindow();
    void updateStatusForMode(RecordMode mode);
    void updateScreenCombo();
    void updateSectionHeights();
    void layoutControlPanel();
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
    QWidget *m_controlPanel = nullptr;
    AudioToggleCard *m_micCard = nullptr;
    AudioToggleCard *m_systemAudioCard = nullptr;
    IRecorderService *m_recorder = nullptr;
    IVideoLibrary *m_library = nullptr;
    QTimer *m_recordingTimer = nullptr;
    QElapsedTimer m_elapsed;
    QTimer *m_countdownTimer = nullptr;
    QTimer *m_stopProgressTimer = nullptr;
    int m_countdownValue = 0;
    bool m_confirmStop = false;
    bool m_darkMode = false;
    paint::Palette m_palette;
    bool m_hiddenForRecording = false;
    bool m_statusOpensSavedVideo = false;
    bool m_regionCommitted = false;
    QString m_lastSavedPath;
    QString m_stopOutputPath;
};
