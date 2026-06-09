#pragma once

#include <QComboBox>

class PaintedComboBox : public QComboBox {
    Q_OBJECT

public:
    explicit PaintedComboBox(QWidget *parent = nullptr);
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
