#include "WindowEnumerator.h"

#include <QMap>

#include <windows.h>

namespace {

struct WindowInfo {
    QString title;
    HWND hwnd;
};

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd) || !GetWindowTextLengthW(hwnd))
        return TRUE;

    wchar_t buf[256];
    GetWindowTextW(hwnd, buf, 256);
    auto *list = reinterpret_cast<QList<WindowInfo> *>(lParam);
    list->append({ QString::fromWCharArray(buf), hwnd });
    return TRUE;
}

}

namespace window_enumerator {

QList<WindowEntry> enumerateWindows()
{
    QList<WindowInfo> windows;
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&windows));

    QMap<QString, int> counts;
    QList<WindowEntry> result;
    for (const auto &info : windows) {
        const QString &original = info.title;
        QString display = original;
        int &count = counts[original];
        if (count > 0)
            display += QStringLiteral(" (%1)").arg(count + 1);
        result.append({ display, original, reinterpret_cast<qulonglong>(info.hwnd) });
        count++;
    }
    return result;
}

}
