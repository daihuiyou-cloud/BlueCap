#pragma once

#include <QString>

#include <windows.h>
#include <shellapi.h>

namespace file_utils {

inline bool moveToTrash(const QString &filePath)
{
    auto *wPath = reinterpret_cast<const wchar_t *>(filePath.utf16());
    std::wstring doubleNull(wPath, filePath.size() + 1);

    SHFILEOPSTRUCTW fos = {};
    fos.wFunc = FO_DELETE;
    fos.pFrom = doubleNull.c_str();
    fos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperationW(&fos) == 0;
}

inline bool isValidFileName(const QString &name)
{
    if (name.isEmpty())
        return false;
    static const QLatin1Char illegal[] = {
        QLatin1Char('/'), QLatin1Char('\\'), QLatin1Char(':'),
        QLatin1Char('*'), QLatin1Char('?'), QLatin1Char('"'),
        QLatin1Char('<'), QLatin1Char('>'), QLatin1Char('|')
    };
    for (auto ch : illegal) {
        if (name.contains(ch))
            return false;
    }
    return true;
}

}
