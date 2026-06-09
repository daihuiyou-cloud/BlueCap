#pragma once

#include <QCache>
#include <QColor>
#include <QFile>
#include <QHash>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QString>

namespace icon {

struct SvgCacheKey {
    QString path;
    QColor color;
    int size;
};

inline bool operator==(const SvgCacheKey &a, const SvgCacheKey &b)
{
    return a.path == b.path && a.color == b.color && a.size == b.size;
}

inline uint qHash(const SvgCacheKey &key, uint seed = 0)
{
    return qHash(key.path, seed) ^ qHash(key.color.name(), seed) ^ (key.size * 1664525u + seed);
}

inline QCache<SvgCacheKey, QPixmap> &svgCache()
{
    static QCache<SvgCacheKey, QPixmap> cache(256);
    return cache;
}

inline QHash<QString, QString> &svgContentCache()
{
    static QHash<QString, QString> cache;
    return cache;
}

inline QPixmap renderSvg(const QString &svgPath, const QColor &color, int size)
{
    const SvgCacheKey key{ svgPath, color, size };
    if (auto *cached = svgCache().object(key))
        return *cached;

    auto &contentCache = svgContentCache();
    auto it = contentCache.constFind(svgPath);
    QString svg;
    if (it != contentCache.constEnd()) {
        svg = it.value();
    } else {
        QFile file(svgPath);
        if (!file.open(QIODevice::ReadOnly))
            return QPixmap(size, size);
        svg = QString::fromUtf8(file.readAll());
        contentCache.insert(svgPath, svg);
    }

    QString coloredSvg = svg;
    coloredSvg.replace(QStringLiteral("currentColor"), color.name());

    QSvgRenderer renderer(coloredSvg.toUtf8());
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);

    svgCache().insert(key, new QPixmap(pixmap));
    return pixmap;
}

inline QIcon coloredIcon(const QString &svgPath, int size,
                         const QColor &normal, const QColor &active, const QColor &disabled)
{
    QIcon icon;
    icon.addPixmap(renderSvg(svgPath, normal, size), QIcon::Normal);
    icon.addPixmap(renderSvg(svgPath, active, size), QIcon::Active);
    icon.addPixmap(renderSvg(svgPath, disabled, size), QIcon::Disabled);
    return icon;
}

}
