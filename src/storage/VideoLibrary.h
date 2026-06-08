#pragma once

#include <QObject>
#include <QStringList>

class VideoLibrary : public QObject
{
    Q_OBJECT

public:
    explicit VideoLibrary(QObject *parent = nullptr);

    QStringList recentVideos() const;

public slots:
    void addRecentVideo(const QString &path);
    void clearAndReplace(const QStringList &videos);

signals:
    void recentVideosChanged(const QStringList &videos);

private:
    QStringList load() const;
    void save(const QStringList &videos) const;
};
