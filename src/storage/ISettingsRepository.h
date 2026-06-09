#pragma once

#include <QVariant>
#include <QString>

class ISettingsRepository
{
public:
    virtual ~ISettingsRepository() = default;

    virtual QVariant value(const QString &key, const QVariant &defaultValue = {}) const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual void sync() = 0;
    virtual void beginGroup(const QString &prefix) = 0;
    virtual void endGroup() = 0;
};
