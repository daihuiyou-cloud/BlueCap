#pragma once

#include <QPushButton>

class ActionButton : public QPushButton
{
    Q_OBJECT
public:
    enum Style { Reset, Browse, Default };
    explicit ActionButton(const QString &text, Style style = Default, QWidget *parent = nullptr);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    Style m_style = Default;
    bool m_darkMode = false;
    bool m_hovered = false;
};
