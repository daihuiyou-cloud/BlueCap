#pragma once

#include "RecordMode.h"

#include <QRect>
#include <QObject>
#include <QString>

class QScreen;

class IRecorderService : public QObject
{
    Q_OBJECT

public:
    explicit IRecorderService(QObject *parent = nullptr) : QObject(parent) {}

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

signals:
    void recordingChanged(bool recording);
    void outputPathChanged(const QString &path);
    void videoSaved(const QString &path);
    void errorOccurred(const QString &message);
    void recordingAreaChanged(const QRect &area, RecordMode mode);
    void recordingWarning(const QString &message);
};
