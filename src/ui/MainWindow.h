#pragma once

#include <QWidget>
#include <QAbstractNativeEventFilter>

class QButtonGroup;
class QLabel;
class QStackedWidget;
class QPoint;
class QPushButton;
class RecorderController;
class RecordPage;
class VideoLibrary;

class MainWindow : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    QWidget *createTitleBar();
    QWidget *createNavBar();
    QPushButton *createWindowButton(const QString &text, const QString &tooltip);
    bool inTitleDragArea(const QPoint &position) const;

    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;
    RecorderController *m_recorder = nullptr;
    VideoLibrary *m_library = nullptr;
    RecordPage *m_recordPage = nullptr;
    QPoint m_dragPosition;
    bool m_dragging = false;
};
