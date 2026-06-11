#pragma once

#include <QFrame>

class BottomBar : public QFrame
{
    Q_OBJECT
public:
    explicit BottomBar(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
};
