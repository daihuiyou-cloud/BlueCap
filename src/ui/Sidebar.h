#pragma once

#include <QWidget>

class QButtonGroup;

class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    void selectPage(int index);

signals:
    void pageSelected(int index);

private:
    QButtonGroup *m_group = nullptr;
};
