#pragma once

#include <QFrame>

class SettingsSeparator : public QFrame
{
    Q_OBJECT
public:
    explicit SettingsSeparator(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
};
