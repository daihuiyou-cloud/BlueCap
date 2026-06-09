#pragma once

#include <QLineEdit>

class SearchLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchLineEdit(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    bool m_darkMode = false;
    bool m_hovered = false;
    bool m_focused = false;
};
