#include "NavigationController.h"

#include <QStackedWidget>

NavigationController::NavigationController(QStackedWidget *stack, QObject *parent)
    : QObject(parent)
    , m_stack(stack)
{
}

void NavigationController::navigate(Page page)
{
    if (!m_stack)
        return;
    m_stack->setCurrentIndex(static_cast<int>(page));
    emit pageChanged(page);
}

Page NavigationController::currentPage() const
{
    if (!m_stack)
        return Page::Record;
    return static_cast<Page>(m_stack->currentIndex());
}
