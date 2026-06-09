#pragma once

#include <QSpinBox>

class PaintedSpinBox : public QSpinBox {
    Q_OBJECT

public:
    explicit PaintedSpinBox(QWidget *parent = nullptr);
    void setDarkMode(bool dark);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_darkMode = false;
    bool m_hovered = false;
    bool m_focused = false;
};
