#pragma once

#include <QMouseEvent>
#include <QPoint>
#include <QWidget>

namespace window_drag {

struct State {
    QPoint dragPosition;
    bool dragging = false;
};

inline bool handlePress(QWidget *target, const QWidget *titleBar,
                         const QPoint &localPos, QMouseEvent *event, State &state)
{
    if (event->button() == Qt::LeftButton && titleBar && titleBar->isVisible()
        && titleBar->geometry().contains(localPos)) {
        state.dragging = true;
        state.dragPosition = event->globalPos() - target->frameGeometry().topLeft();
        event->accept();
        return true;
    }
    return false;
}

inline bool handleMove(QWidget *target, QMouseEvent *event, State &state)
{
    if (state.dragging && (event->buttons() & Qt::LeftButton)) {
        target->move(event->globalPos() - state.dragPosition);
        event->accept();
        return true;
    }
    return false;
}

inline void handleRelease(QMouseEvent *event, State &state)
{
    Q_UNUSED(event);
    state.dragging = false;
}

} // namespace window_drag
