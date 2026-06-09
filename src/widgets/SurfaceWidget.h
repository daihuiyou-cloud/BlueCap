#pragma once

#include <QWidget>

class SurfaceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SurfaceWidget(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
};
