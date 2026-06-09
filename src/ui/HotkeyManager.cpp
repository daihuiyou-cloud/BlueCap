#include "HotkeyManager.h"

#include <QApplication>
#include <QByteArray>

#include <windows.h>

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    qApp->installNativeEventFilter(this);
    m_registered = RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_SHIFT, 'R');
}

HotkeyManager::~HotkeyManager()
{
    qApp->removeNativeEventFilter(this);
    if (m_registered)
        UnregisterHotKey(nullptr, 1);
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    static const QByteArray kMsgGen = QByteArrayLiteral("windows_generic_MSG");
    static const QByteArray kMsgDsp = QByteArrayLiteral("windows_dispatcher_MSG");
    if (eventType != kMsgGen && eventType != kMsgDsp)
        return false;

    auto *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == 1) {
        emit hotkeyPressed();
        if (result)
            *result = 0;
        return true;
    }
    return false;
}
