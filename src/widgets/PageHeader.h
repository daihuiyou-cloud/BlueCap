#pragma once

#include <QLabel>

class PageHeader : public QLabel
{
    Q_OBJECT
public:
    explicit PageHeader(const QString &text, QWidget *parent = nullptr);
    void setDarkMode(bool dark);
private:
    bool m_darkMode = false;
};
