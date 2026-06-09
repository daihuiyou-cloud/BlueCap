#pragma once

#include "ISettingsRepository.h"

#include <QSettings>
#include <QStringList>

class QSettingsRepository : public ISettingsRepository
{
public:
    explicit QSettingsRepository(const QString &organization, const QString &application)
        : m_settings(organization, application)
    {
    }

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const override
    {
        return m_settings.value(key, defaultValue);
    }

    void setValue(const QString &key, const QVariant &value) override
    {
        m_settings.setValue(key, value);
    }

    void sync() override
    {
        m_settings.sync();
    }

    void beginGroup(const QString &prefix) override
    {
        m_settings.beginGroup(prefix);
    }

    void endGroup() override
    {
        m_settings.endGroup();
    }

private:
    mutable QSettings m_settings;
};
