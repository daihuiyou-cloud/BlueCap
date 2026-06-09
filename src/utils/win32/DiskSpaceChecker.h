#pragma once

#include <QString>

#include <windows.h>

namespace disk_space {

inline bool hasAvailableSpace(const QString &path, quint64 requiredBytes)
{
    ULARGE_INTEGER freeBytes;
    if (!GetDiskFreeSpaceExW(path.toStdWString().c_str(), &freeBytes, nullptr, nullptr))
        return true;

    return freeBytes.QuadPart >= requiredBytes;
}

}
