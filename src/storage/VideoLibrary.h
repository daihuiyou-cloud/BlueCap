#pragma once

#include "IVideoLibrary.h"
#include <QStringList>

class ISettingsRepository;

class VideoLibrary : public IVideoLibrary
{
    Q_OBJECT

public:
    explicit VideoLibrary(ISettingsRepository *settings, QObject *parent = nullptr);

    QStringList recentVideos() const override;

    void scanDirectory(const QString &dir) override;

public slots:
    void addRecentVideo(const QString &path) override;
    void removeVideo(const QString &path) override;
    void renameVideo(const QString &oldPath, const QString &newPath) override;
    void clearAndReplace(const QStringList &videos) override;

private:
    void removeNonExistent();
    QStringList load() const;
    void save(const QStringList &videos) const;

    ISettingsRepository *m_settings;
    QStringList m_cache;
};
