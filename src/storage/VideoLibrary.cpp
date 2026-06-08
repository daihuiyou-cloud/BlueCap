#include "VideoLibrary.h"

#include <QSettings>

namespace {
constexpr int kMaxRecentVideos = 8;
}

VideoLibrary::VideoLibrary(QObject *parent)
    : QObject(parent)
{
}

QStringList VideoLibrary::recentVideos() const
{
    return load();
}

void VideoLibrary::addRecentVideo(const QString &path)
{
    QStringList videos = load();
    videos.removeAll(path);
    videos.prepend(path);

    while (videos.size() > kMaxRecentVideos) {
        videos.removeLast();
    }

    save(videos);
    emit recentVideosChanged(videos);
}

QStringList VideoLibrary::load() const
{
    QSettings settings;
    return settings.value(QStringLiteral("library/recentVideos")).toStringList();
}

void VideoLibrary::save(const QStringList &videos) const
{
    QSettings settings;
    settings.setValue(QStringLiteral("library/recentVideos"), videos);
}
