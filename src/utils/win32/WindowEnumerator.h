#pragma once

#include <QList>
#include <QString>

struct WindowEntry {
    QString displayName;
    QString title;
    qulonglong hwnd;
};

namespace window_enumerator {

QList<WindowEntry> enumerateWindows();

}
