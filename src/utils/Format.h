#pragma once

#include <QString>

namespace format {

inline QString fileSize(qint64 bytes)
{
    if (bytes < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(bytes / 1024);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

}
