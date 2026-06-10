#pragma once

#include <QObject>

class QStackedWidget;

enum class Page : int {
    Record = 0,
    Library = 1,
    Settings = 2
};

class NavigationController : public QObject
{
    Q_OBJECT

public:
    explicit NavigationController(QStackedWidget *stack, QObject *parent = nullptr);

    void navigate(Page page);
    Page currentPage() const;

signals:
    void pageChanged(Page page);

private:
    QStackedWidget *m_stack = nullptr;
};
