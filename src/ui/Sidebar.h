#pragma once

#include <QList>
#include <QWidget>

class QButtonGroup;
class QPushButton;

class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    void selectPage(int index);
    void setDarkMode(bool dark);

signals:
    void pageSelected(int index);

private:
    void updateIcons();

    QButtonGroup *m_group = nullptr;
    QList<QPushButton *> m_buttons;
    bool m_darkMode = false;
};
