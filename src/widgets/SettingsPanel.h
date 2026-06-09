#pragma once

#include <QFrame>

class SettingsPanel : public QFrame
{
    Q_OBJECT
public:
    explicit SettingsPanel(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
};
