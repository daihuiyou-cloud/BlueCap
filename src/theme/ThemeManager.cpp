#include "ThemeManager.h"

ThemeManager &ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager()
    : QObject()
{
}

void ThemeManager::setDarkMode(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    emit themeChanged(dark);
    for (auto it = m_updaters.begin(); it != m_updaters.end();) {
        QObject *key = it.key();
        if (!key) {
            it = m_updaters.erase(it);
        } else {
            it.value()(dark);
            ++it;
        }
    }
}

void ThemeManager::registerUpdater(QObject *key, ThemeUpdater updater)
{
    if (!key)
        return;
    m_updaters.remove(key);
    connect(key, &QObject::destroyed, this, [this, key] {
        m_updaters.remove(key);
    });
    updater(m_dark);
    m_updaters.insert(key, std::move(updater));
}
