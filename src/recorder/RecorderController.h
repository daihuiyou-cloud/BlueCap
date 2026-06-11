#pragma once

#include "IRecorderService.h"
#include "recorder/RecordMode.h"

#include <QRect>
#include <QString>
#include <QStringList>

class QScreen;
class StderrMonitor;
class RecordingSession;

class RecorderController : public IRecorderService
{
    Q_OBJECT

public:
    explicit RecorderController(QObject *parent = nullptr);

    bool isRecording() const override;
    QString currentOutputPath() const override;
    QString currentSavePath() const override;

    void setFrameRate(int fps) override;
    void setPreset(const QString &preset) override;
    void setSavePath(const QString &path) override;
    void setShowCursor(bool show) override;
    void setStartTimeout(int ms) override;
    void setStopTimeout(int ms) override;

public slots:
    void startFullScreenRecording(QScreen *screen = nullptr) override;
    void startRegionRecording(const QRect &region) override;
    void startWindowRecording(const QString &windowTitle) override;
    void stopRecording() override;

private slots:
    void handleFinishedCheck();

private:
    void start(const QStringList &args);
    void startCapture(const QString &inputSpec,
                      const QStringList &extraArgs = {},
                      const QStringList &inputArgs = {});

    RecordingSession *m_session = nullptr;
    StderrMonitor *m_stderrMonitor = nullptr;
    QString m_currentOutputPath;
    bool m_errorReported = false;
    int m_frameRate = 30;
    int m_showCursor = true;
    int m_startTimeoutMs = 5000;
    int m_stopTimeoutMs = 5000;
    QString m_preset = QStringLiteral("fast");
    QString m_savePath;
    QString m_encoder;

};
