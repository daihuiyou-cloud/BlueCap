#pragma once

#include <QObject>
#include <QStringList>

class IVideoLibrary : public QObject
{
    Q_OBJECT

public:
    explicit IVideoLibrary(QObject *parent = nullptr) : QObject(parent) {}

    virtual QStringList recentVideos() const = 0;
    virtual void scanDirectory(const QString &dir) = 0;

public slots:
    virtual void addRecentVideo(const QString &path) = 0;
    virtual void clearAndReplace(const QStringList &videos) = 0;

signals:
    void recentVideosChanged(const QStringList &videos);
};
