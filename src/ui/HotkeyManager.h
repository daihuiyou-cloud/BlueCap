#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
    bool isRegistered() const { return m_registered; }

signals:
    void hotkeyPressed();

private:
    bool m_registered = false;
};
