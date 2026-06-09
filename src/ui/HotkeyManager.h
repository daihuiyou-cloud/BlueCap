#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

#include <windows.h>

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
    bool isRegistered() const { return m_registered; }
    void setRecordingHotkeysEnabled(bool enabled);

signals:
    void hotkeyPressed();
    void escapePressed();

private:
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HotkeyManager *s_instance;

    bool m_registered = false;
    HHOOK m_keyboardHook = nullptr;
    bool m_recordingHotkeysEnabled = false;
};
