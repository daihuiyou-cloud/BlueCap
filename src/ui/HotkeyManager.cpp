#include "HotkeyManager.h"

#include <QApplication>
#include <QByteArray>
#include <QMetaObject>

QPointer<HotkeyManager> HotkeyManager::s_instance;

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    qApp->installNativeEventFilter(this);
    m_registered = RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_SHIFT, 'R');
}

HotkeyManager::~HotkeyManager()
{
    if (s_instance.data() == this)
        s_instance.clear();

    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
    qApp->removeNativeEventFilter(this);
    if (m_registered)
        UnregisterHotKey(nullptr, 1);
}

void HotkeyManager::setRecordingHotkeysEnabled(bool enabled)
{
    m_recordingHotkeysEnabled = enabled;
    if (enabled && !m_keyboardHook) {
        m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc,
            GetModuleHandle(nullptr), 0);
    } else if (!enabled && m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
}

LRESULT CALLBACK HotkeyManager::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN && !s_instance.isNull()) {
        auto *kbd = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (kbd->vkCode == VK_ESCAPE && s_instance->m_recordingHotkeysEnabled) {
            QMetaObject::invokeMethod(s_instance, [=] {
                if (!s_instance.isNull() && s_instance->m_recordingHotkeysEnabled)
                    emit s_instance->escapePressed();
            }, Qt::QueuedConnection);
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    static const QByteArray kMsgGen = QByteArrayLiteral("windows_generic_MSG");
    static const QByteArray kMsgDsp = QByteArrayLiteral("windows_dispatcher_MSG");
    if (eventType != kMsgGen && eventType != kMsgDsp)
        return false;

    auto *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY) {
        if (msg->wParam == 1) {
            emit hotkeyPressed();
            if (result)
                *result = 0;
            return true;
        }
    }
    return false;
}
