#pragma once

#include <QRect>
#include <QString>
#include <QObject>

class QScreen;

class IRecorderService
{
public:
    virtual ~IRecorderService() = default;

    virtual bool isRecording() const = 0;
    virtual QString currentOutputPath() const = 0;
    virtual QString currentSavePath() const = 0;

    virtual void startFullScreenRecording(QScreen *screen = nullptr) = 0;
    virtual void startRegionRecording(const QRect &region) = 0;
    virtual void startWindowRecording(const QString &windowTitle) = 0;
    virtual void stopRecording() = 0;

    virtual void setFrameRate(int fps) = 0;
    virtual void setPreset(const QString &preset) = 0;
    virtual void setSavePath(const QString &path) = 0;
    virtual void setShowCursor(bool show) = 0;
    virtual void setStartTimeout(int ms) = 0;
    virtual void setStopTimeout(int ms) = 0;

    virtual QObject *signalSource() const = 0;
};
