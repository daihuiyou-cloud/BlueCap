#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QStackedWidget;
class QPoint;
class QPushButton;
class RecorderController;
class VideoLibrary;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QWidget *createTitleBar();
    QWidget *createNavBar();
    QPushButton *createWindowButton(const QString &text, const QString &tooltip);
    bool inTitleDragArea(const QPoint &position) const;

    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    QPoint m_dragPosition;
    bool m_dragging = false;
};
