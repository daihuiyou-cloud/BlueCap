#pragma once

#include <QHash>
#include <QObject>
#include <functional>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager &instance();

    bool isDark() const { return m_dark; }
    void setDarkMode(bool dark);

    using ThemeUpdater = std::function<void(bool)>;
    void registerUpdater(QObject *key, ThemeUpdater updater);

signals:
    void themeChanged(bool dark);

private:
    ThemeManager();

    QHash<QObject *, ThemeUpdater> m_updaters;
    bool m_dark = false;
};
