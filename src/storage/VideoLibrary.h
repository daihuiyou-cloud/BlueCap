#pragma once

#include <QObject>
#include <QStringList>

class ISettingsRepository;

class VideoLibrary : public QObject
{
    Q_OBJECT

public:
    explicit VideoLibrary(ISettingsRepository *settings, QObject *parent = nullptr);

    QStringList recentVideos() const;

    void scanDirectory(const QString &dir);
    void removeNonExistent();

public slots:
    void addRecentVideo(const QString &path);
    void clearAndReplace(const QStringList &videos);

signals:
    void recentVideosChanged(const QStringList &videos);

private:
    QStringList load() const;
    void save(const QStringList &videos) const;

    ISettingsRepository *m_settings;
    QStringList m_cache;
};
