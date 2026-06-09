#include "VideoLibrary.h"
#include "ISettingsRepository.h"

#include <QDir>
#include <QFileInfo>

namespace {
constexpr int kMaxRecentVideos = 500;
const QLatin1String kLibraryPrefix("library/");
}

VideoLibrary::VideoLibrary(ISettingsRepository *settings, QObject *parent)
    : IVideoLibrary(parent)
    , m_settings(settings)
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

void VideoLibrary::scanDirectory(const QString &dir)
{
    QDir d(dir);
    if (!d.exists())
        return;

    QStringList found;
    for (const auto &fi : d.entryInfoList({ QStringLiteral("*.mp4") }, QDir::Files, QDir::Time)) {
        found.append(fi.absoluteFilePath());
    }

    for (const auto &path : m_cache) {
        if (!found.contains(path) && QFileInfo::exists(path))
            found.append(path);
    }

    if (found != m_cache) {
        m_cache = found;
        while (m_cache.size() > kMaxRecentVideos)
            m_cache.removeLast();
        save(m_cache);
        emit recentVideosChanged(m_cache);
    }
}

void VideoLibrary::removeNonExistent()
{
    int removed = 0;
    for (int i = m_cache.size() - 1; i >= 0; --i) {
        if (!QFileInfo::exists(m_cache[i])) {
            m_cache.removeAt(i);
            ++removed;
        }
    }
    if (removed > 0) {
        save(m_cache);
        emit recentVideosChanged(m_cache);
    }
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
    return m_settings->value(kLibraryPrefix + QLatin1String("recentVideos")).toStringList();
}

void VideoLibrary::save(const QStringList &videos) const
{
    m_settings->setValue(kLibraryPrefix + QLatin1String("recentVideos"), videos);
}
