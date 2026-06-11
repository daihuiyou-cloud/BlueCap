#pragma once

#include <QWidget>

class SurfaceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SurfaceWidget(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
    void setRoundedCorners(bool enabled);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
    bool m_roundedCorners = true;
};
