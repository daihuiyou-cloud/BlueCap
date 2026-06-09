#pragma once

#include <QFrame>

class BottomNavSection : public QFrame
{
    Q_OBJECT
public:
    explicit BottomNavSection(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    bool m_darkMode = false;
    bool m_hovered = false;
};
