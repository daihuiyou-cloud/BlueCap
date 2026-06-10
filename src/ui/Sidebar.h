#pragma once

#include "NavigationController.h"

#include <QList>
#include <QWidget>

class QButtonGroup;
class QPushButton;

class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    void selectPage(Page page);
    void setDarkMode(bool dark);

signals:
    void pageSelected(Page page);

private:
    void updateIcons();

    QButtonGroup *m_group = nullptr;
    QList<QPushButton *> m_buttons;
    bool m_darkMode = false;
};
