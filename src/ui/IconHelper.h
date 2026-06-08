#pragma once

#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QString>

namespace icon {

inline QPixmap renderSvg(const QString &svgPath, const QColor &color, int size)
{
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly))
        return QPixmap(size, size);

    QString svg = QString::fromUtf8(file.readAll());
    svg.replace(QStringLiteral("currentColor"), color.name());

    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
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
