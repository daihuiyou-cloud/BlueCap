#pragma once

#include <QListWidget>

class StyledListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit StyledListWidget(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
};
