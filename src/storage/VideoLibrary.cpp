#include "VideoLibrary.h"

#include <QSettings>

namespace {
constexpr int kMaxRecentVideos = 8;
const QLatin1String kLibraryPrefix("library/");
}

VideoLibrary::VideoLibrary(QObject *parent)
    : QObject(parent)
{
    m_cache = load();
}

QStringList VideoLibrary::recentVideos() const
{
    return m_cache;
}

void VideoLibrary::addRecentVideo(const QString &path)
{
    m_cache.removeAll(path);
    m_cache.prepend(path);

    while (m_cache.size() > kMaxRecentVideos) {
        m_cache.removeLast();
    }

    save(m_cache);
    emit recentVideosChanged(m_cache);
}

void VideoLibrary::clearAndReplace(const QStringList &videos)
{
    m_cache = videos;
    while (m_cache.size() > kMaxRecentVideos) {
        m_cache.removeLast();
    }
    save(m_cache);
    emit recentVideosChanged(m_cache);
}

QStringList VideoLibrary::load() const
{
    QSettings settings;
    return settings.value(kLibraryPrefix + QLatin1String("recentVideos")).toStringList();
}

void VideoLibrary::save(const QStringList &videos) const
{
    QSettings settings;
    settings.setValue(kLibraryPrefix + QLatin1String("recentVideos"), videos);
}
