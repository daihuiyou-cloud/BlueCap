#pragma once

#include <QWidget>

class QLabel;
class ModeSwitch;
class RecordButton;
class RecorderController;
class VideoLibrary;

class RecordPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordPage(RecorderController *recorder, VideoLibrary *library, QWidget *parent = nullptr);

public slots:
    void toggleRecording();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void handleRecordingChanged(bool recording);
    void handleVideoSaved(const QString &path);
    void handleError(const QString &message);
    void updateRecentVideos(const QStringList &videos);

private:
    void startRegionSelection();
    void pickWindow();

    ModeSwitch *m_modeSwitch = nullptr;
    RecordButton *m_recordButton = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_hotkeyLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_recentDetailLabel = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
};
