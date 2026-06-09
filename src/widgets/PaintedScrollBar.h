#pragma once

#include <QScrollBar>

class PaintedScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit PaintedScrollBar(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    bool m_darkMode = false;
    bool m_hovered = false;
    bool m_pressed = false;
};
